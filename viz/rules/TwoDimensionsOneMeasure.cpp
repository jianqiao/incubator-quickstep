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

#include "viz/rules/TwoDimensionsOneMeasure.hpp"

#include "viz/VizAnalyzer.hpp"
#include "viz/VizCounter.hpp"
#include "viz/configs/HeatMap.hpp"
#include "viz/configs/StackedAreaTimeSeries.hpp"
#include "viz/configs/GroupTimeSeries.hpp"
#include "viz/rules/SplitValue.hpp"

namespace quickstep {
namespace viz {

/** \addtogroup Viz
 *  @{
 */

void TwoDimensionsOneMeasure::execute() {
  const AttributeIdVector *dimensions =
      context_->get<AttributeIdVector>("Dimensions");
  CHECK_EQ(2uL, dimensions->getAttributeIds().size());

  const auto &dimension_attr_ids = dimensions->getAttributeIds();
  const AttributeIdVector *measures =
      context_->get<AttributeIdVector>("Measures");
  CHECK_EQ(1uL, measures->getAttributeIds().size());

  const VizCounter *counter =
      context_->get<VizCounter>("VizCounter");
  std::string subgraph = "subgraph" + std::to_string(counter->getCounter());

  std::unique_ptr<VizContext> new_context(new VizContext(context_));
  new_context->set("trace", new StringValue("TwoDimensionsOneMeasure"));
  const VizContextPtr new_context_ptr(new_context.release());

  // TODO: check columns statistics.

  // Try TimeseriesChart
  const VizAnalyzer *analyzer =
      context_->get<VizAnalyzer>("VizAnalyzer");
  for (std::size_t i = 0; i < 2uL; ++i) {
    const attribute_id time_attr_id = dimension_attr_ids.at(i);
    const attribute_id group_attr_id = dimension_attr_ids.at(1-i);
    std::string time_format;
    if (analyzer->isTime(time_attr_id, &time_format)) {
      yield(new GroupTimeSeries(time_attr_id,
                           time_format,
                           group_attr_id,
                           measures->getAttributeIds().front(),
                           new_context_ptr,
                           subgraph + std::to_string(i)));

      yield(new StackedAreaTimeSeries(time_attr_id,
                                      time_format,
                                      group_attr_id,
                                      measures->getAttributeIds().front(),
                                      new_context_ptr,
                                      subgraph + std::to_string(i)));
    }
  }

  // HeatMap
  for (std::size_t i = 0; i < 2uL; ++i) {
      yield(new HeatMap(dimension_attr_ids.at(i),
                        dimension_attr_ids.at(1-i),
                        measures->getAttributeIds().front(),
                        new_context_ptr,
                        subgraph + std::to_string(i)));
  }

  // apply split value
  //derive(new SplitValue(new_context_ptr));
}


/** @} */

}  // namespace viz
}  // namespace quickstep

