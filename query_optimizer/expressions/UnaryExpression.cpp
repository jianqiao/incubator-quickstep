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

#include "query_optimizer/expressions/UnaryExpression.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "expressions/scalar/ScalarUnaryExpression.hpp"
#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/ExprId.hpp"
#include "query_optimizer/expressions/Expression.hpp"
#include "query_optimizer/expressions/PatternMatcher.hpp"
#include "query_optimizer/expressions/Scalar.hpp"
#include "types/operations/unary_operations/UnaryOperation.hpp"
#include "utility/HashPair.hpp"

#include "glog/logging.h"

namespace quickstep {
namespace optimizer {
namespace expressions {

std::string UnaryExpression::getName() const {
  return operation_.getName();
}

ExpressionPtr UnaryExpression::copyWithNewChildren(
    const std::vector<ExpressionPtr> &new_children) const {
  DCHECK_EQ(new_children.size(), children().size());
  DCHECK(SomeScalar::Matches(new_children[0]));
  return UnaryExpression::Create(
      signature_, operation_,
      std::static_pointer_cast<const Scalar>(new_children[0]));
}

::quickstep::Scalar* UnaryExpression::concretize(
    const std::unordered_map<ExprId, const CatalogAttribute*> &substitution_map,
    const std::unordered_set<ExprId> &left_expr_ids,
    const std::unordered_set<ExprId> &right_expr_ids) const {
  return new ::quickstep::ScalarUnaryExpression(
      signature_, operation_,
      operand_->concretize(substitution_map, left_expr_ids, right_expr_ids));
}

std::size_t UnaryExpression::computeHash() const {
  return CombineHashes(signature_->getHash(), operand_->hash());
}

bool UnaryExpression::equals(const ScalarPtr &other) const {
  UnaryExpressionPtr expr;
  if (SomeUnaryExpression::MatchesWithConditionalCast(other, &expr)) {
    return *signature_ == *expr->signature_ && operand_->equals(expr->operand_);
  }
  return false;
}

void UnaryExpression::getFieldStringItems(
    std::vector<std::string> *inline_field_names,
    std::vector<std::string> *inline_field_values,
    std::vector<std::string> *non_container_child_field_names,
    std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
    std::vector<std::string> *container_child_field_names,
    std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const {
  inline_field_names->emplace_back("signature");
  inline_field_values->emplace_back(signature_->toString());

  inline_field_names->emplace_back("result_type");
  inline_field_values->emplace_back(result_type_.getName());

  non_container_child_field_names->push_back("Operand");
  non_container_child_fields->push_back(operand_);
}

}  // namespace expressions
}  // namespace optimizer
}  // namespace quickstep
