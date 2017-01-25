/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 **/

#include "query_execution/WorkOrderProtosContainer.hpp"
#include "query_execution/WorkOrdersContainer.hpp"
#include "relational_operators/UnionAllOperator.hpp"
#include "storage/InsertDestination.hpp"

#include "glog/logging.h"

#include "tmb/id_typedefs.h"

namespace quickstep {

void UnionAllOperator::feedInputBlock(const block_id input_block_id, const relation_id input_relation_id) {
  std::size_t index = relation_id_to_index_.at(input_relation_id);
  if (input_relations_[index]->hasPartitionScheme()) {
    const partition_id part_id =
        input_relations_[index]->getPartitionScheme().getPartitionForBlock(input_block_id);
    input_relations_block_ids_in_partition_[index][part_id].push_back(input_block_id);
  } else {
    input_relations_block_ids_[index].push_back(input_block_id);
  }
}

// The TODO of SelectOperator mentions how to optimize this function
void UnionAllOperator::feedInputBlocks(const relation_id input_relation_id, std::vector<block_id>* input_block_ids) {
  std::size_t index = relation_id_to_index_.at(input_relation_id);
  if (input_relations_[index]->hasPartitionScheme()) {
    for (auto it = input_block_ids->begin(); it != input_block_ids->end(); ++it) {
      const partition_id part_id =
        input_relations_[index]->getPartitionScheme().getPartitionForBlock(*it);
      input_relations_block_ids_in_partition_[index][part_id].push_back(*it);
    }
  } else {
    input_relations_block_ids_[index].insert(input_relations_block_ids_[index].end(),
                                             input_block_ids->begin(),
                                             input_block_ids->end());
  }
}

void UnionAllOperator::doneFeedingInputBlocks(const relation_id rel_id) {
  std::size_t relation_index = relation_id_to_index_.at(rel_id);
  DCHECK(still_feeding_.find(relation_index) != still_feeding_.end());
  still_feeding_.erase(relation_index);
  if (still_feeding_.size() == 0) {
    done_feeding_input_relation_ = true;
  }
}

void UnionAllOperator::addWorkOrdersSingleRelation(
    WorkOrdersContainer *container,
    QueryContext *query_context,
    StorageManager *storage_manager,
    InsertDestination *output_destination,
    std::size_t relation_index) {
  if (input_relation_is_stored_[relation_index]) {
    const std::vector<block_id> &all_blocks = input_relations_block_ids_.at(relation_index);
    for (const block_id input_block_id : all_blocks) {
      container->addNormalWorkOrder(
        new UnionAllWorkOrder(
          query_id_,
          input_relations_[relation_index],
          input_block_id,
          output_destination,
          storage_manager,
          select_attribute_ids_[relation_index]),
        op_index_);
    }
  } else {
    std::size_t num_generated = num_workorders_generated_[relation_index];
    const std::vector<block_id> &all_blocks = input_relations_block_ids_[relation_index];
    while (num_generated < all_blocks .size()) {
      container->addNormalWorkOrder(
        new UnionAllWorkOrder(
          query_id_,
          input_relations_[relation_index],
          all_blocks[num_generated],
          output_destination,
          storage_manager,
          select_attribute_ids_[relation_index]),
        op_index_);
      ++num_generated;
    }
    num_workorders_generated_[relation_index] = num_generated;
  }
}

#ifdef QUICKSTEP_HAVE_LIBNUMA
void UnionAllOperator::addPartitionAwareWorkOrdersSingleRelation(
    WorkOrdersContainer *container,
    QueryContext *query_context,
    StorageManager *storage_manager,
    InsertDestination *output_destination,
    std::size_t relation_index) {
  DCHECK(placement_schemes_[relation_index] != nullptr);
  DCHECK(input_relations_[relation_index]->hasPartitionScheme());
  const std::size_t num_partitions =
    input_relations_[relation_index]->getPartitionScheme().getPartitionSchemeHeader().getNumPartitions();
  if (input_relation_is_stored_[relation_index]) {
    for (std::size_t part_id=0; part_id<num_partitions; part_id++) {
      const std::vector<block_id> &all_blocks =
        input_relations_block_ids_in_partition_[relation_index][part_id];
      for (const block_id input_block_id : all_blocks) {
        container->addNormalWorkOrder(
          new UnionAllWorkOrder(
            query_id_,
            input_relations_[relation_index],
            input_block_id,
            output_destination,
            storage_manager,
            select_attribute_ids_[relation_index]),
          op_index_);
      }
    }
  } else {
    for (std::size_t part_id=0; part_id<num_partitions; part_id++) {
      std::size_t num_generated = num_workorders_generated_in_partition_[relation_index][part_id];
      const std::vector<block_id> &all_blocks = input_relations_block_ids_in_partition_[relation_index][part_id];
      while (num_generated < all_blocks.size()) {
        container->addNormalWorkOrder(
          new UnionAllWorkOrder(
            query_id_,
            input_relations_[relation_index],
            all_blocks[num_generated],
            output_destination,
            storage_manager,
            select_attribute_ids_[relation_index]),
          op_index_);
        ++num_generated;
      }
      num_workorders_generated_in_partition_[relation_index][part_id] = num_generated;
    }
  }
}
#endif

bool UnionAllOperator::getAllWorkOrders(
    WorkOrdersContainer *container,
    QueryContext *query_context,
    StorageManager *storage_manager,
    const tmb::client_id scheduler_client_id,
    tmb::MessageBus *bus) {
  DCHECK(query_context != nullptr);

  InsertDestination *output_destination =
      query_context->getInsertDestination(output_destination_index_);

  // Generate work orders for all the stored relations
  // This block of code will only be executed once
  if (!stored_generated_) {
    for (std::size_t relation_index=0; relation_index<input_relations_.size(); relation_index++) {
      if (input_relation_is_stored_[relation_index]) {
        // Generate work orders for single stored relation
        if (input_relations_[relation_index]->hasPartitionScheme()) {
#ifdef QUICKSTEP_HAVE_LIBNUMA
          if (input_relations_[relation_index]->hasNUMAPlacementScheme()) {
            addPartitionAwareWorkOrdersSingleRelation(container,
                                                      query_context,
                                                      storage_manager,
                                                      output_destination,
                                                      relation_index);
          }
#endif
        } else {
          addWorkOrdersSingleRelation(container,
                                      query_context,
                                      storage_manager,
                                      output_destination,
                                      relation_index);
        }
      }
    }
    stored_generated_ = true;
  }

  // Generate work orders for all the non-stored relations
  for (std::size_t relation_index=0; relation_index<input_relations_.size(); relation_index++) {
    if (!input_relation_is_stored_[relation_index]) {
     if (input_relations_[relation_index]->hasPartitionScheme()) {
#ifdef QUICKSTEP_HAVE_LIBNUMA
       if (input_relations_[relation_index]->hasNUMAPlacementScheme()) {
         addPartitionAwareWorkOrdersSingleRelation(container,
                                                   query_context,
                                                   storage_manager,
                                                   output_destination,
                                                   relation_index);
       }
#endif
     } else {
       addWorkOrdersSingleRelation(container,
                                   query_context,
                                   storage_manager,
                                   output_destination,
                                   relation_index);
     }
    }
  }
  return stored_generated_ && done_feeding_input_relation_;
}

bool UnionAllOperator::getAllWorkOrderProtos(WorkOrderProtosContainer* container) {
  return true;
}

void UnionAllWorkOrder::execute() {
  BlockReference block(
    storage_manager_->getBlock(input_block_id_, *input_relation_));
//    storage_manager_->getBlock(input_block_id_, *input_relation_, getPreferredNUMANodes()[0]));

  block->selectSimple(select_attribute_id_,
                      nullptr,
                      output_destination_);
}

}  // namespace quickstep
