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

#include "query_optimizer/logical/Deduplicate.hpp"

#include <string>
#include <vector>

#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/ExpressionUtil.hpp"
#include "query_optimizer/expressions/PatternMatcher.hpp"
#include "utility/Cast.hpp"

#include "glog/logging.h"

namespace quickstep {
namespace optimizer {
namespace logical {

namespace E = ::quickstep::optimizer::expressions;

LogicalPtr Deduplicate::copyWithNewChildren(
    const std::vector<LogicalPtr> &new_children) const {
  DCHECK_EQ(children().size(), new_children.size());
  return Deduplicate::Create(new_children[0], attribute_list_);
}

std::vector<E::AttributeReferencePtr> Deduplicate::getOutputAttributes() const {
  return attribute_list_;
}

std::vector<E::AttributeReferencePtr> Deduplicate::getReferencedAttributes() const {
  return attribute_list_;
}

LogicalPtr Deduplicate::copyWithNewInputExpressions(
    const std::vector<E::ExpressionPtr> &input_expressions) const {
  DCHECK_EQ(attribute_list_.size(), input_expressions.size());
  std::vector<E::AttributeReferencePtr> new_attribute_list;
  for (const E::ExpressionPtr &input_expression : input_expressions) {
    E::AttributeReferencePtr attribute;
    E::SomeAttributeReference::MatchesWithConditionalCast(input_expression, &attribute);
    DCHECK(attribute != nullptr);
    new_attribute_list.emplace_back(attribute);
  }
  return Create(input_, new_attribute_list);
}

void Deduplicate::getFieldStringItems(
    std::vector<std::string> *inline_field_names,
    std::vector<std::string> *inline_field_values,
    std::vector<std::string> *non_container_child_field_names,
    std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
    std::vector<std::string> *container_child_field_names,
    std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const {
  non_container_child_field_names->push_back("input");
  non_container_child_fields->push_back(input_);

  container_child_field_names->push_back("attribute_list");
  container_child_fields->push_back(
      CastSharedPtrVector<OptimizerTreeBase>(attribute_list_));
}

}  // namespace logical
}  // namespace optimizer
}  // namespace quickstep
