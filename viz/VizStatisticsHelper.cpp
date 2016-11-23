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

// On OSX, ranlib complains if a static library archive contains no symbols,
// so we export a dummy global variable.

#include "viz/VizStatisticsHelper.hpp"

#include <sstream>
#include <string>

#include "catalog/CatalogAttribute.hpp"
#include "catalog/CatalogRelation.hpp"
#include "cli/DropRelation.hpp"
#include "parser/ParseStatement.hpp"
#include "parser/SqlParserWrapper.hpp"
#include "query_optimizer/QueryHandle.hpp"
#include "query_optimizer/QueryProcessor.hpp"
#include "types/TypedValue.hpp"

namespace quickstep {
namespace viz {

void VizStatisticsHelper::getStatistics(const client_id main_thread_client_id,
                                        const client_id foreman_client_id,
                                        MessageBus *bus,
                                        StorageManager *storage_manager,
                                        QueryProcessor *query_processor,
                                        const CatalogRelation *query_result_relation,
                                        RelationStatistics *stat) {
  stat->num_distinct_values_.clear();
  stat->min_values_.clear();
  stat->max_values_.clear();

  std::unique_ptr<SqlParserWrapper> parser_wrapper(new SqlParserWrapper());
  for(const CatalogAttribute &attribute : *query_result_relation) {
    std::ostringstream query_builder("SELECT COUNT(DISTINCT ");
    query_builder<<attribute.getName()<<"), MIN(";
    query_builder<<attribute.getName()<<"), MAX(";
    query_builder<<attribute.getName()<<") FROM ";
    query_builder<<query_result_relation->getName()<<";";

    std::vector<TypedValue> results =
      executeQueryForSingleRow(main_thread_client_id,
                                    foreman_client_id,
                                    query_builder.str(),
                                    bus,
                                    storage_manager,
                                    query_processor,
                                    parser_wrapper.get());
    DCHECK_EQ(3u, results.size());
    DCHECK(results[0].getTypeID() == TypeID::kLong);
    stat->num_distinct_values_.push_back(results[0].getLiteral<std::int64_t>());
    stat->min_values_.push_back(results[1]);
    stat->max_values_.push_back(results[2]);
  }

  std::string query_string = "SELECT COUNT(*) FROM ";
  query_string.append(query_result_relation->getName());
  query_string.append(";");
  TypedValue num_tuples =
    executeQueryForSingleResult(main_thread_client_id,
                                    foreman_client_id,
                                    query_string,
                                    bus,
                                    storage_manager,
                                    query_processor,
                                    parser_wrapper.get());
  DCHECK(num_tuples.getTypeID() == TypeID::kLong);
  stat->num_tuples_ = num_tuples.getLiteral<std::int64_t>();
}

TypedValue VizStatisticsHelper::executeQueryForSingleResult(
    const client_id main_thread_client_id,
    const client_id foreman_client_id,
    const std::string &query_string,
    MessageBus *bus,
    StorageManager *storage_manager,
    QueryProcessor *query_processor,
    SqlParserWrapper *parser_wrapper) {
  std::vector<TypedValue> results =
      executeQueryForSingleRow(main_thread_client_id,
                               foreman_client_id,
                               query_string,
                               bus,
                               storage_manager,
                               query_processor,
                               parser_wrapper);
  DCHECK_EQ(1u, results.size());
  return results[0];
}

std::vector<TypedValue> VizStatisticsHelper::executeQueryForSingleRow(
    const client_id main_thread_client_id,
    const client_id foreman_client_id,
    const std::string &query_string,
    MessageBus *bus,
    StorageManager *storage_manager,
    QueryProcessor *query_processor,
    SqlParserWrapper *parser_wrapper) {
  parser_wrapper->feedNextBuffer(new std::string(query_string));

  ParseResult result = parser_wrapper->getNextStatement();
  DCHECK(result.condition == ParseResult::kSuccess);

  const ParseStatement &statement = *result.parsed_statement;

  // Generate the query plan.
  std::unique_ptr<QueryHandle> query_handle(
      std::make_unique<QueryHandle>(query_processor->query_id(),
                                    main_thread_client_id,
                                    statement.getPriority()));
  query_processor->generateQueryHandle(statement, query_handle.get());
  DCHECK(query_handle->getQueryPlanMutable() != nullptr);

  // Use foreman to execute the query plan.
  QueryExecutionUtil::ConstructAndSendAdmitRequestMessage(
      main_thread_client_id, foreman_client_id, query_handle.get(), bus);

  QueryExecutionUtil::ReceiveQueryCompletionMessage(main_thread_client_id, bus);

  // Retrieve the scalar result from the result relation.
  const CatalogRelation *query_result_relation = query_handle->getQueryResultRelation();
  DCHECK(query_result_relation != nullptr);

  std::vector<TypedValue> values;
  {
    std::vector<block_id> blocks = query_result_relation->getBlocksSnapshot();
    DCHECK_EQ(1u, blocks.size());

    BlockReference block = storage_manager->getBlock(blocks[0], *query_result_relation);
    const TupleStorageSubBlock &tuple_store = block->getTupleStorageSubBlock();
    DCHECK_EQ(1, tuple_store.numTuples());

    const std::size_t num_columns = tuple_store.getRelation().size();
    if (tuple_store.isPacked()) {
      for (std::size_t i = 0; i < num_columns; ++i) {
        values.emplace_back(tuple_store.getAttributeValueTyped(0, i));
        values[i].ensureNotReference();
      }
    } else {
      std::unique_ptr<TupleIdSequence> existence_map(tuple_store.getExistenceMap());
      for (std::size_t i = 0; i < num_columns; ++i) {
        values.emplace_back(
            tuple_store.getAttributeValueTyped(*existence_map->begin(), i));
        values[i].ensureNotReference();
      }
    }
  }

  // Drop the result relation.
  DropRelation::Drop(*query_result_relation,
                     query_processor->getDefaultDatabase(),
                     storage_manager);

  return values;
}


} // namespace viz
} // namespace quickstep

