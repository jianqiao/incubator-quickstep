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

#include "relational_operators/UnionAllOperator.hpp"
#include "storage/InsertDestination.hpp"

#include "glog/logging.h"

#include "tmb/id_typedefs.h"

namespace quickstep {

void UnionAllOperator::feedInputBlock(const block_id input_block_id, const relation_id input_relation_id) override {
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
void UnionAllOperator::feedInputBlocks(const relation_id input_relation_id, std::vector<block_id>* input_block_ids) override {
  std::size_t index = relation_id_to_index_.at(input_relation_id);
  if (input_relations_.hasPartitionScheme()) {
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

void UnionAllOperator::doneFeedingInputBlocks(const relation_id rel_id) override {
  DCHECK(still_feeding_.find(rel_id) != still_feeding_.end());
  still_feeding_.erase(rel_id);
  if (still_feeding_.size() == 0) {
    done_feeding_input_relation_ = true;
  }
}

bool UnionAllOperator::getAllWorkOrders(
    WorkOrdersContainer *container,
    QueryContext *query_context,
    StorageManager *storage_manager,
    const tmb::client_id scheduler_client_id,
    tmb::MessageBus *bus) override {
  DCHECK(query_context != nullptr);

  InsertDestination *output_destination =
      query_context->getInsertDestination(output_destination_index_);

  for (std::size_t i=0; i<input_relations_.size(); i++) {
    if (input_relation_is_stored_[i]) {

    }
  }
}

bool UnionAllOperator::getAllWorkOrdersProtos(WorkOrderProtosContainer* container) override {

}

}  // namespace quickstep
