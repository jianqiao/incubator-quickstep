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
#ifndef QUICKSTEP_VIZ_VIZ_STATISTICS_HELPER_HPP_
#define QUICKSTEP_VIZ_VIZ_STATISTICS_HELPER_HPP_

#include <vector>

#include "query_execution/QueryExecutionTypedefs.hpp"
#include "types/TypedValue.hpp"
#include "utility/Macros.hpp"

namespace quickstep{

class CatalogRelation;
class QueryProcessor;
class StorageManager;

namespace viz{

/** \addtogroup Viz
 *  @{
 */

class RelationStatistics{
 public:
  std::size_t num_tuples_;
  std::vector<std::size_t> num_distinct_values_;
  std::vector<TypedValue> min_values_;
  std::vector<TypedValue> max_values_;
};

class VizStatisticsHelper{
 public:
  static void getStatistics(const client_id main_thread_client_id,
                            const client_id foreman_client_id,
                            MessageBus *bus,
                            StorageManager* storage_manager,
                            QueryProcessor *query_processor,
                            const CatalogRelation *query_result_relation,
                            RelationStatistics *stat);

 private:
  VizStatisticsHelper() {}

  DISALLOW_COPY_AND_ASSIGN(VizStatisticsHelper);
};


/** @} */

} // namespace viz
} // namespace quickstep

#endif  // QUICKSTEP_VIZ_VIZ_STATISTICS_HELPER_HPP_

