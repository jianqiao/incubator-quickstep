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

#include "query_optimizer/rules/InjectJoinFilters.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "query_optimizer/cost_model/StarSchemaSimpleCostModel.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/expressions/ExpressionUtil.hpp"
#include "query_optimizer/physical/LIPFilterConfiguration.hpp"
#include "query_optimizer/physical/Aggregate.hpp"
#include "query_optimizer/physical/FilterInjection.hpp"
#include "query_optimizer/physical/HashJoin.hpp"
#include "query_optimizer/physical/PatternMatcher.hpp"
#include "query_optimizer/physical/Physical.hpp"
#include "query_optimizer/physical/PhysicalType.hpp"
#include "query_optimizer/physical/Selection.hpp"
#include "query_optimizer/physical/TopLevelPlan.hpp"
#include "query_optimizer/rules/PruneColumns.hpp"
#include "types/TypeID.hpp"
#include "types/TypedValue.hpp"
#include "utility/lip_filter/LIPFilter.hpp"

#include "glog/logging.h"

namespace quickstep {
namespace optimizer {

namespace E = ::quickstep::optimizer::expressions;
namespace P = ::quickstep::optimizer::physical;

P::PhysicalPtr InjectJoinFilters::apply(const P::PhysicalPtr &input) {
  DCHECK(input->getPhysicalType() == P::PhysicalType::kTopLevelPlan);

  const P::TopLevelPlanPtr top_level_plan =
     std::static_pointer_cast<const P::TopLevelPlan>(input);
  cost_model_.reset(
      new cost::StarSchemaSimpleCostModel(
          top_level_plan->shared_subplans()));
  lip_filter_configuration_.reset(new P::LIPFilterConfiguration());

  P::PhysicalPtr output = transformHashJoinToFilters(input);
  output = pushDownFilters(output);
  output = addFilterAnchors(output, false);
  output = PruneColumns().apply(output);

  concretizeAsLIPFilters(output, nullptr);

  if (!lip_filter_configuration_->getBuildInfoMap().empty() ||
      !lip_filter_configuration_->getProbeInfoMap().empty()) {
    output = std::static_pointer_cast<const P::TopLevelPlan>(output)
        ->copyWithLIPFilterConfiguration(
              P::LIPFilterConfigurationPtr(lip_filter_configuration_.release()));
  }

  return output;
}

bool InjectJoinFilters::isTransformable(
    const physical::HashJoinPtr &hash_join) const {
  if (hash_join->residual_predicate() != nullptr) {
    return false;
  }
  if (hash_join->right_join_attributes().size() > 1) {
    return false;
  }
  if (!E::SubsetOfExpressions(hash_join->getOutputAttributes(),
                              hash_join->left()->getOutputAttributes())) {
    return false;
  }
  switch (hash_join->join_type()) {
    case P::HashJoin::JoinType::kInnerJoin: {
      if (!cost_model_->impliesUniqueAttributes(hash_join->right(),
                                                hash_join->right_join_attributes())) {
        return false;
      }
      break;
    }
    case P::HashJoin::JoinType::kLeftSemiJoin:  // Fall through
    case P::HashJoin::JoinType::kLeftAntiJoin:
      break;
    default:
      return false;
  }

  std::int64_t min_cpp_value;
  std::int64_t max_cpp_value;
  const bool has_min_max_stats =
      findMinMaxValuesForAttributeHelper(hash_join->right(),
                                         hash_join->right_join_attributes().front(),
                                         &min_cpp_value,
                                         &max_cpp_value);
  if (!has_min_max_stats) {
    return false;
  }

  // TODO(jianqiao): implement SimpleHashSetExactFilter to relax this requirement.
  // 1G bits = 128MB
  if (min_cpp_value < 0 || max_cpp_value > kMaxFilterSize) {
    return false;
  }

  return true;
}

P::PhysicalPtr InjectJoinFilters::transformHashJoinToFilters(
    const P::PhysicalPtr &input) const {
  std::vector<P::PhysicalPtr> new_children;
  bool has_changed_children = false;
  for (const P::PhysicalPtr &child : input->children()) {
    const P::PhysicalPtr new_child = transformHashJoinToFilters(child);
    if (child != new_child && !has_changed_children) {
      has_changed_children = true;
    }
    new_children.push_back(new_child);
  }

  P::HashJoinPtr hash_join;
  if (P::SomeHashJoin::MatchesWithConditionalCast(input, &hash_join) &&
      isTransformable(hash_join)) {
    const bool is_anti_join =
        hash_join->join_type() == P::HashJoin::JoinType::kLeftAntiJoin;

    P::PhysicalPtr build_child = new_children[1];
    E::PredicatePtr build_side_filter_predicate = nullptr;
    P::SelectionPtr selection;
    if (P::SomeSelection::MatchesWithConditionalCast(build_child, &selection) &&
        E::SubsetOfExpressions(hash_join->right_join_attributes(),
                               selection->input()->getOutputAttributes())) {
      build_child = selection->input();
      build_side_filter_predicate = selection->filter_predicate();
    }

    return P::FilterInjection::Create(new_children[0],
                                      build_child,
                                      hash_join->left_join_attributes(),
                                      hash_join->right_join_attributes(),
                                      hash_join->project_expressions(),
                                      build_side_filter_predicate,
                                      is_anti_join);
  }

  if (has_changed_children) {
    return input->copyWithNewChildren(new_children);
  } else {
    return input;
  }
}

physical::PhysicalPtr InjectJoinFilters::pushDownFilters(
    const physical::PhysicalPtr &input) const {
  std::vector<P::PhysicalPtr> new_children;
  bool has_changed_children = false;
  for (const P::PhysicalPtr &child : input->children()) {
    const P::PhysicalPtr new_child = pushDownFilters(child);
    if (child != new_child && !has_changed_children) {
      has_changed_children = true;
    }
    new_children.push_back(new_child);
  }

  P::FilterInjectionPtr filter_injection;
  if (P::SomeFilterInjection::MatchesWithConditionalCast(input, &filter_injection)) {
    DCHECK_EQ(2u, new_children.size());
    return pushDownFiltersInternal(
        new_children[0], new_children[1], filter_injection);
  }

  if (has_changed_children) {
    return input->copyWithNewChildren(new_children);
  } else {
    return input;
  }
}

physical::PhysicalPtr InjectJoinFilters::pushDownFiltersInternal(
    const physical::PhysicalPtr &probe_child,
    const physical::PhysicalPtr &build_child,
    const physical::FilterInjectionPtr &filter_injection) const {
  switch (probe_child->getPhysicalType()) {
    case P::PhysicalType::kAggregate:  // Fall through
    case P::PhysicalType::kHashJoin:
    case P::PhysicalType::kSample:
    case P::PhysicalType::kSelection:
    case P::PhysicalType::kSort:
    case P::PhysicalType::kWindowAggregate: {
      DCHECK_GE(probe_child->getNumChildren(), 1u);
      const P::PhysicalPtr child = probe_child->children()[0];
      if (E::SubsetOfExpressions(filter_injection->probe_attributes(),
                                 child->getOutputAttributes())) {
        const P::PhysicalPtr new_child =
            pushDownFiltersInternal(child, build_child, filter_injection);
        if (new_child != child) {
          std::vector<P::PhysicalPtr> new_children = probe_child->children();
          new_children[0] = new_child;
          return probe_child->copyWithNewChildren(new_children);
        }
      }
    }
    default:
      break;
  }

  if (probe_child != filter_injection->left()) {
    // TODO(jianqiao): may need to update probe_attributes.
    return P::FilterInjection::Create(probe_child,
                                      build_child,
                                      filter_injection->probe_attributes(),
                                      filter_injection->build_attributes(),
                                      E::ToNamedExpressions(probe_child->getOutputAttributes()),
                                      filter_injection->build_side_filter_predicate(),
                                      filter_injection->is_anti_filter());
  } else {
    return filter_injection;
  }
}


physical::PhysicalPtr InjectJoinFilters::addFilterAnchors(
    const physical::PhysicalPtr &input,
    const bool ancestor_can_anchor_filter) const {
  std::vector<P::PhysicalPtr> new_children;

  switch (input->getPhysicalType()) {
    case P::PhysicalType::kAggregate: {
      const P::AggregatePtr &aggregate =
          std::static_pointer_cast<const P::Aggregate>(input);
      new_children.emplace_back(
          addFilterAnchors(aggregate->input(), true));
      break;
    }
    case P::PhysicalType::kSelection: {
      const P::SelectionPtr &selection =
          std::static_pointer_cast<const P::Selection>(input);
      new_children.emplace_back(
          addFilterAnchors(selection->input(), true));
      break;
    }
//    case P::PhysicalType::kHashJoin: {
//      const P::HashJoinPtr &hash_join =
//          std::static_pointer_cast<const P::HashJoin>(input);
//      new_children.emplace_back(
//          addFilterAnchors(hash_join->left(), true));
//      new_children.emplace_back(
//          addFilterAnchors(hash_join->right(), false));
//      break;
//    }
    case P::PhysicalType::kFilterInjection: {
      const P::FilterInjectionPtr &filter_injection =
          std::static_pointer_cast<const P::FilterInjection>(input);
      new_children.emplace_back(
          addFilterAnchors(filter_injection->left(), true));
      new_children.emplace_back(
          addFilterAnchors(filter_injection->right(), true));
      break;
    }
    default: {
      for (const P::PhysicalPtr &child : input->children()) {
        new_children.emplace_back(addFilterAnchors(child, false));
      }
    }
  }

  DCHECK_EQ(new_children.size(), input->children().size());
  const P::PhysicalPtr output_with_new_children =
      new_children == input->children()
          ? input
          : input->copyWithNewChildren(new_children);

  if (input->getPhysicalType() == P::PhysicalType::kFilterInjection &&
      !ancestor_can_anchor_filter) {
    const P::FilterInjectionPtr &filter_injection =
        std::static_pointer_cast<const P::FilterInjection>(output_with_new_children);
    return P::Selection::Create(filter_injection,
                                filter_injection->project_expressions(),
                                nullptr);
  } else {
    return output_with_new_children;
  }
}

void InjectJoinFilters::concretizeAsLIPFilters(
    const P::PhysicalPtr &input,
    const P::PhysicalPtr &anchor_node) const {
  switch (input->getPhysicalType()) {
    case P::PhysicalType::kAggregate: {
      const P::AggregatePtr &aggregate =
          std::static_pointer_cast<const P::Aggregate>(input);
      concretizeAsLIPFilters(aggregate->input(), aggregate);
      break;
    }
    case P::PhysicalType::kSelection: {
      const P::SelectionPtr &selection =
          std::static_pointer_cast<const P::Selection>(input);
      concretizeAsLIPFilters(selection->input(), selection);
      break;
    }
//    case P::PhysicalType::kHashJoin: {
//      const P::HashJoinPtr &hash_join =
//          std::static_pointer_cast<const P::HashJoin>(input);
//      concretizeAsLIPFilters(hash_join->left(), hash_join);
//      concretizeAsLIPFilters(hash_join->right(), nullptr);
//      break;
//    }
    case P::PhysicalType::kFilterInjection: {
      const P::FilterInjectionPtr &filter_injection =
          std::static_pointer_cast<const P::FilterInjection>(input);
      DCHECK_EQ(1u, filter_injection->build_attributes().size());
      const E::AttributeReferencePtr &build_attr =
          filter_injection->build_attributes().front();

      std::int64_t min_cpp_value;
      std::int64_t max_cpp_value;
      const bool has_min_max_stats =
          findMinMaxValuesForAttributeHelper(filter_injection,
                                             build_attr,
                                             &min_cpp_value,
                                             &max_cpp_value);
      DCHECK(has_min_max_stats);
      DCHECK_GE(min_cpp_value, 0);
      DCHECK_GE(max_cpp_value, 0);
      DCHECK_LE(max_cpp_value, kMaxFilterSize);
      CHECK(anchor_node != nullptr);

      lip_filter_configuration_->addBuildInfo(
          build_attr,
          filter_injection,
          static_cast<std::size_t>(max_cpp_value),
          LIPFilterType::kBitVectorExactFilter,
          filter_injection->is_anti_filter());
      lip_filter_configuration_->addProbeInfo(
          filter_injection->probe_attributes().front(),
          anchor_node,
          build_attr,
          filter_injection);

      concretizeAsLIPFilters(filter_injection->left(), anchor_node);
      concretizeAsLIPFilters(filter_injection->right(), filter_injection);
      break;
    }
    default: {
      for (const P::PhysicalPtr &child : input->children()) {
        concretizeAsLIPFilters(child, nullptr);
      }
    }
  }
}

bool InjectJoinFilters::findMinMaxValuesForAttributeHelper(
    const physical::PhysicalPtr &physical_plan,
    const expressions::AttributeReferencePtr &attribute,
    std::int64_t *min_cpp_value,
    std::int64_t *max_cpp_value) const {
  const TypedValue min_value =
      cost_model_->findMinValueStat(physical_plan, attribute);
  const TypedValue max_value =
      cost_model_->findMaxValueStat(physical_plan, attribute);
  if (min_value.isNull() || max_value.isNull()) {
    return false;
  }

  switch (attribute->getValueType().getTypeID()) {
    case TypeID::kInt: {
      *min_cpp_value = min_value.getLiteral<int>();
      *max_cpp_value = max_value.getLiteral<int>();
      return true;
    }
    case TypeID::kLong: {
      *min_cpp_value = min_value.getLiteral<std::int64_t>();
      *max_cpp_value = max_value.getLiteral<std::int64_t>();
      return true;
    }
    default:
      return false;
  }
}


}  // namespace optimizer
}  // namespace quickstep
