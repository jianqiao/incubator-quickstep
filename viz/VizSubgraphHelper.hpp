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

#ifndef QUICKSTEP_VIZ_VIZ_SUBGRAPH_HELPER_HPP_
#define QUICKSTEP_VIZ_VIZ_SUBGRAPH_HELPER_HPP_

#include <vector>
#include <unordered_map>

#include "utility/Macros.hpp"
#include "viz/configs/VizConfig.hpp"

#include "json.hpp"

namespace quickstep {
namespace viz {

using nlohmann::json;

/** \addtogroup Viz
 *  @{
 */

class VizSubgraphHelper {
 public:
  static void groupingSubgraphs(std::vector<std::unique_ptr<VizConfig>> &confs, json& viz);

 private:
  VizSubgraphHelper() {}

  static std::unordered_map<std::string, bool> checkGroup
            (const std::unordered_map<std::string, std::vector<std::unique_ptr<VizConfig>>> &group);

  DISALLOW_COPY_AND_ASSIGN(VizSubgraphHelper);
 };

/** @} */

}  // namespace viz
}  // namespace quickstep

#endif  // QUICKSTEP_VIZ_VIZ_SUBGRAPH_HELPER_HPP_
