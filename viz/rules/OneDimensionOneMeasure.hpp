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

#ifndef QUICKSTEP_VIZ_RULES_ONE_DIMENSION_ONE_MEASURE_HPP_
#define QUICKSTEP_VIZ_RULES_ONE_DIMENSION_ONE_MEASURE_HPP_

#include "utility/Macros.hpp"
#include "viz/VizContext.hpp"
#include "viz/rules/VizRule.hpp"

namespace quickstep {
namespace viz {

/** \addtogroup Viz
 *  @{
 */

class OneDimensionOneMeasure : public VizRule {
 public:
  OneDimensionOneMeasure(const VizContextPtr &context)
      : VizRule(context) {}

  ~OneDimensionOneMeasure() override {}

  void execute() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OneDimensionOneMeasure);
 };

/** @} */

}  // namespace viz
}  // namespace quickstep

#endif  // QUICKSTEP_VIZ_RULES_ONE_DIMENSION_ONE_MEASURE_HPP_
