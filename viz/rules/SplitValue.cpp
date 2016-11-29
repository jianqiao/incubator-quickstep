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

#include "viz/rules/SplitValue.hpp"

#include <algorithm>

#include "utility/Macros.hpp"
#include "viz/VizLiteralHelper.hpp"
#include "viz/VizStatisticsHelper.hpp"
#include "viz/VizAnalyzer.hpp"
#include "viz/rules/OneDimensionMultiMeasures.hpp"
#include "viz/rules/OneDimensionOneMeasure.hpp"
#include "viz/rules/TwoDimensionsOneMeasure.hpp"

namespace quickstep {
namespace viz {

/** \addtogroup Viz
 *  @{
 */

void SplitValue::execute() {
  const VizAnalyzer *analyzer =
      context_->get<VizAnalyzer>("VizAnalyzer");

  // dimensions stay the same, only update the measures
  const AttributeIdVector *dimensions =
      context_->get<AttributeIdVector>("Dimensions");
  const AttributeIdVector *measures =
      context_->get<AttributeIdVector>("Measures");

  // check if maximum values of measures differ too much
  std::vector<std::pair<double, attribute_id> > literal;
  if (checkInBoundary(analyzer, measures, literal))
    return;

  std::vector<std::vector<attribute_id>> split;
  splitAttributes(literal, split);

  for (std::size_t i=0; i<split.size(); i++) {
    std::unique_ptr<VizContext> new_context(new VizContext(context_));
    new_context->set("trace", new StringValue("SplitValue"));
    new_context->set("Measures",
                     new AttributeIdVector(split[i]));
    dispatchWithGrouping(analyzer,
                         VizContextPtr(new_context.release()),
                         dimensions->getAttributeIds().size(),
                         split[i].size());
  }
}

void SplitValue::dispatchWithGrouping(const VizAnalyzer *analyzer,
                                      const VizContextPtr new_context,
                                      const std::size_t num_dimension_attrs,
                                      const std::size_t num_measure_attrs) {
  if (num_dimension_attrs == 1) {
    if (num_measure_attrs == 1) {
      derive(new OneDimensionOneMeasure(new_context));
    } else {
      derive(new OneDimensionMultiMeasures(new_context));
    }
  } else if(num_dimension_attrs == 2) {
    if (num_measure_attrs == 1) {
      derive(new TwoDimensionsOneMeasure(new_context));
    }
  }
}

bool SplitValue::checkInBoundary(const VizAnalyzer *analyzer,
                                 const AttributeIdVector *measures,
                                 std::vector<std::pair<double, attribute_id>>& literal) {
  const RelationStatistics *stat = analyzer->getRelationStatistics();
  for (const attribute_id column_id : measures->getAttributeIds()) {
    double var = VizLiteralHelper::getLiteral(stat->max_values_[column_id]);
    literal.push_back(std::make_pair(var, column_id));
  }

  // debug
  for(int i=0; i<literal.size(); i++)
    std::cout<<literal[i].first<<" "<<literal[i].second<<std::endl;
  
  std::pair<double, attribute_id> maxx = *std::min_element(literal.begin(), literal.end());
  std::pair<double, attribute_id> minn = *std::max_element(literal.begin(), literal.end());
  // check if the max of max_values_ is 10 times the min of max_values, then need to split
  if (maxx.first > minn.first * kDiff)
    return false;
  return true;
}

void SplitValue::splitAttributes(std::vector<std::pair<double, attribute_id>> &literal,
                                 std::vector<std::vector<attribute_id>> &split) {
  sort(literal.begin(), literal.end());
  split.clear();

  std::vector<std::pair<double, attribute_id>>::const_iterator pos = literal.begin();
  for (std::vector<std::pair<double, attribute_id>>::const_iterator iter = pos; iter != literal.end(); ++iter) {
    if ((iter->first) > (pos->first) * kDiff) {
      std::vector<attribute_id> temp;
      for (std::vector<std::pair<double, attribute_id>>::const_iterator it = pos; it != iter; ++it)
        temp.push_back(it->second);
      split.push_back(std::move(temp));
      pos = iter;
    }
  }
  
  if (pos != literal.end()) {
    std::vector<attribute_id> temp;
    for (std::vector<std::pair<double, attribute_id>>::const_iterator it = pos; it != literal.end(); ++it)
      temp.push_back(it->second);
    split.push_back(std::move(temp));
  }
}

/** @} */

}  // namespace viz
}  // namespace quickstep

