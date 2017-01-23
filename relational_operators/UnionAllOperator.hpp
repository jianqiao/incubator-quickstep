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

#ifndef QUICKSTEP_RELATIONAL_OPERATORS_UNION_ALL_OPERATOR_HPP_
#define QUICKSTEP_RELATIONAL_OPERATORS_UNION_ALL_OPERATOR_HPP_

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "catalog/CatalogRelation.hpp"

#ifdef QUICKSTEP_HAVE_LIBNUMA
#include "catalog/NUMAPlacementScheme.hpp"
#endif

#include "relational_operators/RelationalOperator.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

#include "tmb/id_typedefs.h"

namespace tmb { class MessageBus; }

namespace quickstep {

class QueryContext;
class StorageManager;
class WorkOrderProtosContainer;
class WorkOrdersContainer;

/** \addtogroup RelationalOperators
 *  @{
 */

/**
 * @brief An operator which performs UNION ALL operation to relational tables.
 **/
class UnionAllOperator : public RelationalOperator {
 public:
  /**
   * @brief Constructor.
   *
   * @param query_id The ID of the query to which this operator belongs.
   * @param relation The relation to add. This CreateTableOperator owns
   *        relation until the WorkOrder it produces is successfully executed,
   *        at which point it is owned by database.
   * @param database The database to add a relation to.
   **/
  UnionAllOperator(const std::size_t query_id,
                   const std::vector<CatalogRelation*> &input_relations,
                   const CatalogRelation &output_relation,
                   const QueryContext::insert_destination_id output_destination_index,
                   const std::vector<bool> &input_relation_is_stored,
                   const std::vector<std::vector<attribute_id>> &select_attribute_ids)
      : RelationalOperator(query_id),
        input_relations_(input_relations),
        input_relation_is_stored_(input_relation_is_stored),
        output_relation_(output_relation),
        output_destination_index_(output_destination_index),
        select_attribute_ids_(select_attribute_ids),
        stored_generated_(false) {
    // for every input relation, do the initialization
    for (std::size_t i=0; i<input_relation.size(); i++) {
#ifdef QUICKSTEP_HAVE_LIBNUMA
      placement_schemes_.push_back(input_relations[i]->getNUMAPlacementSchemePtr());
#endif
      relation_id_to_index_.emplace(input_relations[i]->getID(), i);
      if (!input_relation_is_stored[i]) {
        still_feeding_.add(i);
      }

      // initialize block_ids and num_workorders with partition
      input_relations_block_ids_.push_back(input_relation_is_stored[i]
                                           ? input_relations[i]->getBlocksSnapshot()
                                           : std::vector<block_id>());
      num_workorders_generated_.push_back(0);

      // initialize block_ids and num_workorders with partition
      if (input_relations[i]->hasPartitionScheme()) {
        const PartitionScheme &part_scheme = input_relations[i]->getPartitionScheme();
        const PartitionSchemeHeader &part_scheme_header = part_scheme.getPartitionSchemeHeader();
        const std::size_t num_partitions = part_scheme_header.getNumPartitions();

        input_relations_block_ids_in_partition_.push_back(std::vector<std::vector<block_id>>(num_partitions));
        num_workorders_generated_in_partition_.push_back(std::vector<std::size_t>(num_partitions, 0));

        std::vector<std::vector<block_id>>& partition = input_relations_block_ids_in_partition_.back();
        for (std::size_t part_id=0; part_id<num_partitions; part_id++) {
          if (input_relation_is_stored[i]) {
            partition[part_id] = part_scheme.getBlocksInPartition(part_id);
          }  else {
            partition[part_id] = std::vector<block_id>();
          }
        }
      } else {
        // if does not have PartitionScheme, push back an empty vector
        input_relations_block_ids_in_partition_.push_back(std::vector<std::vector<block_id>>());
        num_workorders_generated_in_partition_.push_back(std::vector<std::size_t>());
      }
    }
  }

  ~UnionAllOperator() override {}

  std::string getName() const override {
    return "UnionAll";
  }

  const std::vector<CatalogRelation*>& input_relations() const {
    return input_relations_;
  }

  const CatalogRelation& output_relation() const {
    return output_relation_;
  }

  QueryContext::insert_destination_id getInsertDestinationID() const override {
    return output_destination_index_;
  }

  const relation_id getOutputRelationID() const override {
    return output_relation_.getID();
  }

  void feedInputBlock(const block_id input_block_id,
                      const relation_id input_relation_id) override;

  void feedInputBlocks(const relation_id input_relation_id,
                      std::vector<block_id> *input_block_ids) override;

  void doneFeedingInputBlocks(const relation_id rel_id) override;

  void addWorkOrdersSingleRelation(
          WorkOrdersContainer *container,
          QueryContext *query_context,
          StorageManager *storage_manager,
          InsertDestination *output_destination,
          std::size_t relation_index);

#ifdef QUICKSTEP_HAVE_LIBNUMA
  void addPartitionAwareWorkOrdersSingleRelation(
          WorkOrdersContainer *container,
          QueryContext *query_context,
          StorageManager *storage_manager,
          InsertDestination *output_destination,
          std::size_t relation_index);
#endif

  bool getAllWorkOrders(WorkOrdersContainer *container,
                        QueryContext *query_context,
                        StorageManager *storage_manager,
                        const tmb::client_id scheduler_client_id,
                        tmb::MessageBus *bus) override;

  bool getAllWorkOrdersProtos(WorkOrderProtosContainer *container) override;

 private:

#ifdef QUICKSTEP_HAVE_LIBNUMA
  std::vector<const NUMAPlacementScheme*> placement_schemes_;
#endif

  const std::vector<CatalogRelation*> &input_relations_;
  const std::vector<bool> &input_relation_is_stored_;

  const CatalogRelation &output_relation_;
  const QueryContext::insert_destination_id output_destination_index_;

  // block ids, in partition or not
  std::vector< std::vector<block_id> > input_relations_block_ids_;
  std::vector< std::vector<std::vector<block_id>> > input_relations_block_ids_in_partition_;

  // number of workorders generated, in partition or not
  std::vector<std::size_t> num_workorders_generated_;
  std::vector< std::vector<std::size_t> > num_workorders_generated_in_partition_;

  // attributes for projection
  const std::vector< std::vector<attribute_id> > select_attribute_ids_;

  // Relation indices that are not stored, and are still feeding
  std::unordered_set<std::size_t> still_feeding_;
  // If all the stored relations are generated
  bool stored_generated_;

  // map from relation_id to index in vector
  std::unordered_map<const relation_id, std::size_t> relation_id_to_index_;

  DISALLOW_COPY_AND_ASSIGN(UnionAllOperator);
};


/**
 * A work order produced by UnionAllOperator
 */

class UnionAllWorkOrder : public WorkOrder {
 public:

  /**
   * @brief Constructor
   */
  UnionAllWorkOrder(const std::size_t query_id,
                    const CatalogRelationSchema *input_relation,
                    const block_id input_block_id,
                    InsertDestination *output_destination,
                    StorageManager *storage_manager,
                    const std::vector<attribute_id> &select_attribute_id)
      : WorkOrder(query_id),
        input_relation_(input_relation),
        input_block_id_(input_block_id),
        output_destination_(output_destination),
        storage_manager_(storage_manager),
        select_attribute_id_(select_attribute_id) {

  }

  /**
   * @brief Constructor for the distributed setting
   */

  /*
  UnionAllWorkOrder(const std::size_t query_id,
                    const block_id input_block_id,
                    StorageManager *storage_manager)
      : WorkOrder(query_id),
        input_block_id_(input_block_id),
        storage_manager_(storage_manager) {

  }
  */

  ~UnionAllWorkOrder() override {}

  void execute() override;

 private:

  const CatalogRelationSchema *input_relation_;
  const block_id input_block_id_;

  InsertDestination *output_destination_;
  StorageManager* storage_manager_;

  const std::vector<attribute_id> select_attribute_id_;

  DISALLOW_COPY_AND_ASSIGN(UnionAllWorkOrder);
}

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_RELATIONAL_OPERATORS_UNION_ALL_OPERATOR_HPP_
