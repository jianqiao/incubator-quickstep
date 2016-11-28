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

#include "utility/Macros.hpp"
#include "viz/VizLiteralHelper.hpp"
#include "viz/VizStatisticsHelper.hpp"
#include "viz/VizAnalyzer.hpp"

namespace quickstep {
namespace viz {

/** \addtogroup Viz
 *  @{
 */

class SplitValue : public VizRule {
 public:
  SplitValue(const VizContextPtr &context)
      : VizRule(context) {}

  ~SplitValue() override {}

  void execute() override {
    const VizAnalyzer *analyzer =
        context_->get<VizAnalyzer>("VizAnalyzer");

    const AttributeIdVector *measures =
        context_->get<AttributeIdVector>("Measures");

    std::vector<double> literal;
    if (checkInBoundary(analyzer,measures, literal)){
      return;
    }

  }

 protected:
  bool checkInBoundary(const VizAnalyzer *analyzer, const AttributeIdVector *measures, std::vector<double>& literal) {
    // check if the maximum value does not differ too much
    const RelationStatistics *stat = analyzer->getRelationStatistics();
    for (const attribute_id column_id : measures->getAttributeIds()) {
      literal.push_back(VizLiteralHelper::getLiteral(stat->max_values_[column_id]));
    }
    return true;
  }

 private:

  DISALLOW_COPY_AND_ASSIGN(SplitValue);
 };

/** @} */

}  // namespace viz
}  // namespace quickstep

#endif  // QUICKSTEP_VIZ_RULES_SPLIT_VALUE_HPP_
