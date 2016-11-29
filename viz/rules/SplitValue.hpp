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

#ifndef QUICKSTEP_VIZ_RULES_SPLIT_VALUE_HPP_
#define QUICKSTEP_VIZ_RULES_SPLIT_VALUE_HPP_

#include <algorithm>

#include "utility/Macros.hpp"
#include "viz/rules/Grouping.hpp"

namespace quickstep {
namespace viz {

class VizAnalyzer;

/** \addtogroup Viz
 *  @{
 */

class SplitValue : public Grouping {
 public:
  SplitValue(const VizContextPtr &context)
      : Grouping(context) {}

  ~SplitValue() override {}

  void execute() override;

 protected:

  bool checkInBoundary(const VizAnalyzer *analyzer,
                       const AttributeIdVector *measures,
                       std::vector<std::pair<double, attribute_id>>& literal);

  void splitAttributes(std::vector<std::pair<double, attribute_id>> &literal,
                       std::vector<std::vector<attribute_id>> &split);

 private:
  // check if the max of max_values_ is kDiff times the min of max_values_, then need to split
  static constexpr double kDiff = 10.0;

  DISALLOW_COPY_AND_ASSIGN(SplitValue);
 };

/** @} */

}  // namespace viz
}  // namespace quickstep

#endif  // QUICKSTEP_VIZ_RULES_SPLIT_VALUE_HPP_
