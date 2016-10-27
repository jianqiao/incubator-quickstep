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

#ifndef QUICKSTEP_QUERY_OPTIMIZER_RULES_INJECT_JOIN_FILTERS_HPP_
#define QUICKSTEP_QUERY_OPTIMIZER_RULES_INJECT_JOIN_FILTERS_HPP_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "query_optimizer/cost_model/StarSchemaSimpleCostModel.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/expressions/ExprId.hpp"
#include "query_optimizer/physical/LIPFilterConfiguration.hpp"
#include "query_optimizer/physical/FilterInjection.hpp"
#include "query_optimizer/physical/HashJoin.hpp"
#include "query_optimizer/physical/Physical.hpp"
#include "query_optimizer/rules/Rule.hpp"
#include "utility/Macros.hpp"

namespace quickstep {
namespace optimizer {

/** \addtogroup OptimizerRules
 *  @{
 */

class InjectJoinFilters : public Rule<physical::Physical> {
 public:
  /**
   * @brief Constructor.
   */
  InjectJoinFilters() {}

  ~InjectJoinFilters() override {}

  std::string getName() const override {
    return "TransformFilterJoins";
  }

  physical::PhysicalPtr apply(const physical::PhysicalPtr &input) override;

 private:
  bool isTransformable(const physical::HashJoinPtr &hash_join) const;

  physical::PhysicalPtr transformHashJoinToFilters(
      const physical::PhysicalPtr &input) const;

  physical::PhysicalPtr pushDownFilters(const physical::PhysicalPtr &input) const;

  physical::PhysicalPtr addFilterAnchors(const physical::PhysicalPtr &input,
                                         const bool ancestor_can_anchor_filter) const;

  void concretizeAsLIPFilters(const physical::PhysicalPtr &input,
                              const physical::PhysicalPtr &anchor_node) const;

  physical::PhysicalPtr pushDownFiltersInternal(
      const physical::PhysicalPtr &probe_child,
      const physical::PhysicalPtr &build_child,
      const physical::FilterInjectionPtr &filter_injection) const;

  bool findMinMaxValuesForAttributeHelper(
      const physical::PhysicalPtr &physical_plan,
      const expressions::AttributeReferencePtr &attribute,
      std::int64_t *min_cpp_value,
      std::int64_t *max_cpp_value) const;

  std::unique_ptr<cost::StarSchemaSimpleCostModel> cost_model_;
  std::unique_ptr<physical::LIPFilterConfiguration> lip_filter_configuration_;

  // 1G bits = 128MB
  static constexpr std::int64_t kMaxFilterSize = 1000000000;

  DISALLOW_COPY_AND_ASSIGN(InjectJoinFilters);
};

/** @} */

}  // namespace optimizer
}  // namespace quickstep

#endif  // QUICKSTEP_QUERY_OPTIMIZER_RULES_INJECT_JOIN_FILTERS_HPP_
