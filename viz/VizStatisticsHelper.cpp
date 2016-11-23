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
#include "parser/SqlParserWrapper.hpp"

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

} // namespace viz
} // namespace quickstep

