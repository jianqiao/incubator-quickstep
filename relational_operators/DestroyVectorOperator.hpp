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

#ifndef QUICKSTEP_RELATIONAL_OPERATORS_DESTROY_VECTOR_OPERATOR_HPP_
#define QUICKSTEP_RELATIONAL_OPERATORS_DESTROY_VECTOR_OPERATOR_HPP_

#include <string>

#include "catalog/CatalogTypedefs.hpp"
#include "query_execution/QueryContext.hpp"
#include "relational_operators/RelationalOperator.hpp"
#include "relational_operators/WorkOrder.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

#include "tmb/id_typedefs.h"

namespace tmb { class MessageBus; }

namespace quickstep {

class StorageManager;
class WorkOrderProtosContainer;
class WorkOrdersContainer;

/** \addtogroup RelationalOperators
 *  @{
 */

/**
 * @brief An operator which destroys a shared hash table.
 **/
class DestroyVectorOperator : public RelationalOperator {
 public:
  /**
   * @brief Constructor.
   *
   * @param query_id The ID of the query to which this operator belongs.
   * @param build_num_partitions The number of partitions in 'build_relation'.
   * @param hash_table_index The index of the CollisionFreeVector in QueryContext.
   **/
  DestroyVectorOperator(const std::size_t query_id,
                        const std::size_t build_num_partitions,
                        const QueryContext::join_hash_table_id hash_table_index)
      : RelationalOperator(query_id),
        build_num_partitions_(build_num_partitions),
        hash_table_index_(hash_table_index),
        work_generated_(false) {}

  ~DestroyVectorOperator() override {}

  OperatorType getOperatorType() const override {
    return kDestroyVector;
  }

  std::string getName() const override {
    return "DestroyVectorOperator";
  }

  bool getAllWorkOrders(WorkOrdersContainer *container,
                        QueryContext *query_context,
                        StorageManager *storage_manager,
                        const tmb::client_id scheduler_client_id,
                        tmb::MessageBus *bus) override;

  bool getAllWorkOrderProtos(WorkOrderProtosContainer *container) override;

 private:
  const std::size_t build_num_partitions_;
  const QueryContext::join_hash_table_id hash_table_index_;
  bool work_generated_;

  DISALLOW_COPY_AND_ASSIGN(DestroyVectorOperator);
};

/**
 * @brief A WorkOrder produced by DestroyVectorOperator.
 **/
class DestroyVectorWorkOrder : public WorkOrder {
 public:
  /**
   * @brief Constructor.
   *
   * @param query_id The ID of the query to which this WorkOrder belongs.
   * @param hash_table_index The index of the CollisionFreeVector in QueryContext.
   * @param part_id The partition id.
   * @param query_context The QueryContext to use.
   **/
  DestroyVectorWorkOrder(const std::size_t query_id,
                         const QueryContext::join_hash_table_id hash_table_index,
                         const partition_id part_id,
                         QueryContext *query_context)
      : WorkOrder(query_id),
        hash_table_index_(hash_table_index),
        part_id_(part_id),
        query_context_(DCHECK_NOTNULL(query_context)) {}

  ~DestroyVectorWorkOrder() override {}

  void execute() override;

 private:
  const QueryContext::join_hash_table_id hash_table_index_;
  const partition_id part_id_;
  QueryContext *query_context_;

  DISALLOW_COPY_AND_ASSIGN(DestroyVectorWorkOrder);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_RELATIONAL_OPERATORS_DESTROY_VECTOR_OPERATOR_HPP_
