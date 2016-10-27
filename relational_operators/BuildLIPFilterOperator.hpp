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

#ifndef QUICKSTEP_RELATIONAL_OPERATORS_BUILD_LIP_FILTER_OPERATOR_HPP_
#define QUICKSTEP_RELATIONAL_OPERATORS_BUILD_LIP_FILTER_OPERATOR_HPP_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/CatalogRelation.hpp"
#include "catalog/CatalogTypedefs.hpp"
#include "query_execution/QueryContext.hpp"
#include "relational_operators/RelationalOperator.hpp"
#include "relational_operators/WorkOrder.hpp"
#include "storage/StorageBlockInfo.hpp"
#include "utility/Macros.hpp"
#include "utility/lip_filter/LIPFilterAdaptiveProber.hpp"
#include "utility/lip_filter/LIPFilterBuilder.hpp"

#include "glog/logging.h"

#include "tmb/id_typedefs.h"

namespace tmb { class MessageBus; }

namespace quickstep {

class CatalogRelationSchema;
class Predicate;
class StorageManager;
class WorkOrderProtosContainer;
class WorkOrdersContainer;

namespace serialization { class WorkOrder; }

/** \addtogroup RelationalOperators
 *  @{
 */

/**
 * @brief An operator which builds a LIPFilter on one relation.
 **/
class BuildLIPFilterOperator : public RelationalOperator {
 public:
  BuildLIPFilterOperator(const std::size_t query_id,
                         const CatalogRelation &input_relation,
                         const QueryContext::predicate_id build_side_predicate_index,
                         const bool input_relation_is_stored)
    : RelationalOperator(query_id),
      input_relation_(input_relation),
      build_side_predicate_index_(build_side_predicate_index),
      input_relation_is_stored_(input_relation_is_stored),
      input_relation_block_ids_(input_relation_is_stored ? input_relation.getBlocksSnapshot()
                                                         : std::vector<block_id>()),
      num_workorders_generated_(0),
      started_(false) {}

  ~BuildLIPFilterOperator() override {}

  const CatalogRelation& input_relation() const {
    return input_relation_;
  }

  std::string getName() const override {
    return "BuildLIPFilterOperator";
  }

  bool getAllWorkOrders(WorkOrdersContainer *container,
                        QueryContext *query_context,
                        StorageManager *storage_manager,
                        const tmb::client_id scheduler_client_id,
                        tmb::MessageBus *bus) override;

  bool getAllWorkOrderProtos(WorkOrderProtosContainer *container) override;

  void feedInputBlock(const block_id input_block_id,
                      const relation_id input_relation_id) override {
    input_relation_block_ids_.push_back(input_block_id);
  }

  void feedInputBlocks(const relation_id rel_id,
                       std::vector<block_id> *partially_filled_blocks) override {
    input_relation_block_ids_.insert(input_relation_block_ids_.end(),
                                     partially_filled_blocks->begin(),
                                     partially_filled_blocks->end());
  }

 private:
  /**
   * @brief Create Work Order proto.
   *
   * @param block The block id used in the Work Order.
   **/
  serialization::WorkOrder* createWorkOrderProto(const block_id block);

  const CatalogRelation &input_relation_;
  const QueryContext::predicate_id build_side_predicate_index_;
  const bool input_relation_is_stored_;

  std::vector<block_id> input_relation_block_ids_;
  std::vector<block_id>::size_type num_workorders_generated_;

  bool started_;

  DISALLOW_COPY_AND_ASSIGN(BuildLIPFilterOperator);
};

/**
 * @brief A WorkOrder produced by BuildLIPFilterOperator
 **/
class BuildLIPFilterWorkOrder : public WorkOrder {
 public:
  BuildLIPFilterWorkOrder(const std::size_t query_id,
                          const CatalogRelationSchema &input_relation,
                          const block_id build_block_id,
                          const Predicate *build_side_predicate,
                          StorageManager *storage_manager,
                          LIPFilterAdaptiveProber *lip_filter_adaptive_prober,
                          LIPFilterBuilder *lip_filter_builder)
      : WorkOrder(query_id),
        input_relation_(input_relation),
        build_block_id_(build_block_id),
        build_side_predicate_(build_side_predicate),
        storage_manager_(DCHECK_NOTNULL(storage_manager)),
        lip_filter_adaptive_prober_(lip_filter_adaptive_prober),
        lip_filter_builder_(DCHECK_NOTNULL(lip_filter_builder)) {}

  ~BuildLIPFilterWorkOrder() override {}

  const CatalogRelationSchema& input_relation() const {
    return input_relation_;
  }

  void execute() override;

 private:
  const CatalogRelationSchema &input_relation_;
  const block_id build_block_id_;
  const Predicate *build_side_predicate_;

  StorageManager *storage_manager_;

  std::unique_ptr<LIPFilterAdaptiveProber> lip_filter_adaptive_prober_;
  std::unique_ptr<LIPFilterBuilder> lip_filter_builder_;

  DISALLOW_COPY_AND_ASSIGN(BuildLIPFilterWorkOrder);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_RELATIONAL_OPERATORS_BUILD_LIP_FILTER_OPERATOR_HPP_
