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

#include "query_optimizer/rules/CollapseSelection.hpp"

#include <vector>

#include "query_optimizer/expressions/Expression.hpp"
#include "query_optimizer/expressions/LogicalAnd.hpp"
#include "query_optimizer/expressions/NamedExpression.hpp"
#include "query_optimizer/expressions/PatternMatcher.hpp"
#include "query_optimizer/expressions/Predicate.hpp"
#include "query_optimizer/physical/Aggregate.hpp"
#include "query_optimizer/physical/PatternMatcher.hpp"
#include "query_optimizer/physical/Physical.hpp"
#include "query_optimizer/physical/PhysicalType.hpp"
#include "query_optimizer/physical/Selection.hpp"
#include "query_optimizer/rules/RuleHelper.hpp"

namespace quickstep {
namespace optimizer {

namespace E = ::quickstep::optimizer::expressions;
namespace P = ::quickstep::optimizer::physical;

P::PhysicalPtr CollapseSelection::applyToNode(const P::PhysicalPtr &input) {
  switch (input->getPhysicalType()) {
    case P::PhysicalType::kAggregate: {
      const P::AggregatePtr parent =
          std::static_pointer_cast<const P::Aggregate>(input);
      P::SelectionPtr child;
      if (P::SomeSelection::MatchesWithConditionalCast(parent->input(), &child)) {
        return applyToAggregateSelection(parent, child);
      }
      break;
    }
    case P::PhysicalType::kSelection: {
      const P::SelectionPtr parent =
          std::static_pointer_cast<const P::Selection>(input);
      P::SelectionPtr child;
      if (P::SomeSelection::MatchesWithConditionalCast(parent->input(), &child)) {
        return applyToSelectionSelection(parent, child);
      }
      break;
    }
    default:
      break;
  }

  return input;
}

P::PhysicalPtr CollapseSelection::applyToSelectionSelection(
    const P::SelectionPtr &parent, const P::SelectionPtr &child) const {
  E::PredicatePtr filter_predicate = parent->filter_predicate();

  std::vector<E::ExpressionPtr> non_project_expressions;
  if (filter_predicate != nullptr) {
    non_project_expressions.push_back(filter_predicate);
  }

  std::vector<E::NamedExpressionPtr> project_expressions = parent->project_expressions();
  PullUpProjectExpressions(child->project_expressions(),
                           { &non_project_expressions },
                           { &project_expressions });

  const E::PredicatePtr &child_filter_predicate = child->filter_predicate();
  if (filter_predicate != nullptr) {
    if (child_filter_predicate != nullptr) {
      filter_predicate = E::LogicalAnd::Create({ filter_predicate, child_filter_predicate });
    }
  } else {
    filter_predicate = child_filter_predicate;
  }

  return P::Selection::Create(child->input(),
                              project_expressions,
                              filter_predicate,
                              parent->cloneOutputPartitionSchemeHeader());
}

P::PhysicalPtr CollapseSelection::applyToAggregateSelection(
    const P::AggregatePtr &parent, const P::SelectionPtr &child) const {
  // This is for avoiding potential performance degrading, i.e. common derived
  // attributes that would be evaluated in a duplicated manner. Nevertheless the
  // case can be further optimized if more careful checking gets applied.
  if (!parent->aggregate_expressions().empty() || parent->filter_predicate() != nullptr) {
    return parent;
  }

  std::vector<E::NamedExpressionPtr> grouping_expressions = parent->grouping_expressions();
  PullUpProjectExpressions(child->project_expressions(), {}, { &grouping_expressions });

  return P::Aggregate::Create(child->input(),
                              grouping_expressions,
                              {} /* aggregate_expressions */,
                              child->filter_predicate(),
                              parent->hasRepartition(),
                              parent->cloneOutputPartitionSchemeHeader());
}

}  // namespace optimizer
}  // namespace quickstep
