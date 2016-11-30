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
  static void groupingSubgraphs(std::vector<std::unique_ptr<VizConfig>> &confs, json& viz) {
    // use name of VizConfig to group subgraphs
    std::unordered_map<std::string, std::vector<std::unique_ptr<VizConfig>> > group;
    for (auto &conf : confs) {
      std::string name = conf->getName();
      group[name].emplace_back(conf.release());
    }

    // check if group of subgraph meets some condition
    std::unordered_map<std::string, bool> check = checkGroup(group);

    // generate JSON if a group pass the check
    for (const auto &pair : group) {
      if (check.at(pair.first) == false)
        continue;
      json graph = json::array();
      for (const auto &conf : pair.second) {
        graph.push_back(conf->toJSON());
      }
      viz.push_back(graph);
    }
  }

 private:
  VizSubgraphHelper() {}

  std::unordered_map<std::string, bool> checkGroup(const std::unordered_map<std::string, std::vector<std::unique_ptr<VizConfig>>> &group) {
    std::unordered_map<string, int> measure_size;
    int max_size = -1;
    for (const auto &pair : group) {
      int sz = 0;
      for (const auto &conf : pair.second)
        sz += conf->getNumMeasure();
      measure_size[pair.first] = sz;
      if (sz > max_size)
        max_size = sz;
    }
    std::unordered_map<std::string, bool> check;
    for (const auto &pair : measure_size)
      check[pair.first] = pair.second == max_size ? true : false;
    return check;
  }

  DISALLOW_COPY_AND_ASSIGN(VizSubgraphHelper);
 };

/** @} */

}  // namespace viz
}  // namespace quickstep

#endif  // QUICKSTEP_VIZ_VIZ_SUBGRAPH_HELPER_HPP_
