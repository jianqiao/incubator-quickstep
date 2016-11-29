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

#include "viz/rules/OneDimensionMultiMeasures.hpp"

#include "viz/VizAnalyzer.hpp"
#include "viz/configs/BarChart.hpp"
#include "viz/configs/LineChart.hpp"
#include "viz/rules/SplitValue.hpp"

namespace quickstep {
namespace viz {

/** \addtogroup Viz
 *  @{
 */


void OneDimensionMultiMeasures::execute() {
  const AttributeIdVector *dimensions =
      context_->get<AttributeIdVector>("Dimensions");
  CHECK_EQ(1uL, dimensions->getAttributeIds().size());

  const AttributeIdVector *measures =
      context_->get<AttributeIdVector>("Measures");

  std::unique_ptr<VizContext> new_context(new VizContext(context_));
  new_context->set("trace", new StringValue("OneDimensionMultiMeasures"));
  const VizContextPtr new_context_ptr(new_context.release());

  // Barchart
  yield(new BarChart(dimensions->getAttributeIds().front(),
                     measures->getAttributeIds(),
                     new_context_ptr));

  // LineChart
  yield(new LineChart(dimensions->getAttributeIds().front(),
                      measures->getAttributeIds(),
                      new_context_ptr));

  derive(new SplitValue(new_context_ptr));
}

/** @} */

}  // namespace viz
}  // namespace quickstep

