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
#include <string>
#include <memory>

#include "catalog/CatalogRelation.hpp"
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
                   const std::vector<CatalogRelation> &input_relations,
                   const CatalogRelation &output_relation)
      : RelationalOperator(query_id),
        input_relations_(input_relations),
        output_relation_(output_relation) {
  }

  ~UnionAllOperator() override {}

  std::string getName() const override {
    return "UnionAll";
  }

  const std::vector<CatalogRelation>& input_relations() const {
    return input_relations_;
  }

  const CatalogRelation& output_relation() const {
    return output_relation_;
  }

  bool getAllWorkOrders(WorkOrdersContainer *container,
                        QueryContext *query_context,
                        StorageManager *storage_manager,
                        const tmb::client_id scheduler_client_id,
                        tmb::MessageBus *bus) override;

  bool getAllWorkOrdersProtos(WorkOrderProtosContainer *container) override;

  void addWorkOrders(WorkOrdersContainer *container,
                     QueryContext *query_context,
                     StorageManager *storage_manager,
                     InsertDestination *output_destination);

 private:

  const std::vector<CatalogRelation> &input_relations_;
  const CatalogRelation &output_relation_;

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
                    const block_id input_block_id,
                    StorageManager *storage_manager)
      : WorkOrder(query_id),
        input_block_id_(input_block_id),
        storage_manager_(storage_manager) {

  }

  UnionAllWorkOrder(const std::size_t query_id,
                    const block_id input_block_id,
                    StorageManager *storage_manager)
      : WorkOrder(query_id),
        input_block_id_(input_block_id),
        storage_manager_(storage_manager) {

  }

 private:

  const block_id input_block_id_;

  StorageManager* storage_manager_;

  DISALLOW_COPY_AND_ASSIGN(UnionAllWorkOrder);
}

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_RELATIONAL_OPERATORS_UNION_ALL_OPERATOR_HPP_
