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

#include "query_optimizer/expressions/BinaryExpression.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "expressions/scalar/ScalarBinaryExpression.hpp"
#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/expressions/ExprId.hpp"
#include "query_optimizer/expressions/Expression.hpp"
#include "query_optimizer/expressions/PatternMatcher.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperatorPrecedence.hpp"
#include "types/operations/binary_operations/BinaryOperation.hpp"
#include "utility/HashPair.hpp"

#include "glog/logging.h"

namespace quickstep {

class Type;

namespace optimizer {
namespace expressions {

BinaryExpression::BinaryExpression(const OperationSignaturePtr &signature,
                                   const BinaryOperation &operation,
                                   const ScalarPtr &left,
                                   const ScalarPtr &right)
    : signature_(signature),
      operation_(operation),
      left_(left),
      right_(right),
      result_type_(operation_.resultTypeForSignature(signature_)) {
  DCHECK(operation_.canApplyToSignature(signature_)) << toString();
  addChild(left_);
  addChild(right_);
}

std::string BinaryExpression::getName() const {
  return operation_.getName();
}

std::pair<std::string, std::size_t> BinaryExpression::generateNameWithPrecedence() const {
  const auto left_info = left_->generateNameWithPrecedence();
  const auto right_info = right_->generateNameWithPrecedence();
  const std::string name = operation_.formatExpression(signature_,
                                                       left_info.first,
                                                       left_info.second,
                                                       right_info.first,
                                                       right_info.second);
  return std::make_pair(name, operation_.getOperatorPrecedence());
}

ExpressionPtr BinaryExpression::copyWithNewChildren(
    const std::vector<ExpressionPtr> &new_children) const {
  DCHECK_EQ(new_children.size(), children().size());
  DCHECK(SomeScalar::Matches(new_children[0]));
  DCHECK(SomeScalar::Matches(new_children[1]));
  return BinaryExpression::Create(
      signature_,
      operation_,
      std::static_pointer_cast<const Scalar>(new_children[0]),
      std::static_pointer_cast<const Scalar>(new_children[1]));
}

std::vector<AttributeReferencePtr> BinaryExpression::getReferencedAttributes() const {
  std::vector<AttributeReferencePtr> referenced_attributes =
      left_->getReferencedAttributes();
  const std::vector<AttributeReferencePtr> referenced_attributes_by_right_operand =
      right_->getReferencedAttributes();
  referenced_attributes.insert(referenced_attributes.end(),
                               referenced_attributes_by_right_operand.begin(),
                               referenced_attributes_by_right_operand.end());
  return referenced_attributes;
}

::quickstep::Scalar *BinaryExpression::concretize(
    const std::unordered_map<ExprId, const CatalogAttribute*> &substitution_map,
    const std::unordered_set<ExprId> &left_expr_ids,
    const std::unordered_set<ExprId> &right_expr_ids) const {
  return new ::quickstep::ScalarBinaryExpression(
      signature_, operation_,
      left_->concretize(substitution_map, left_expr_ids, right_expr_ids),
      right_->concretize(substitution_map, left_expr_ids, right_expr_ids));
}

std::size_t BinaryExpression::computeHash() const {
  std::size_t left_hash = left_->hash();
  std::size_t right_hash = right_->hash();

  if (operation_.isCommutative() && left_hash > right_hash) {
    std::swap(left_hash, right_hash);
  }

  return CombineHashes(signature_->getHash(),
                       CombineHashes(left_hash, right_hash));
}

bool BinaryExpression::equals(const ScalarPtr &other) const {
  BinaryExpressionPtr expr;
  if (SomeBinaryExpression::MatchesWithConditionalCast(other, &expr) &&
      *signature_ == *expr->signature_) {
    ScalarPtr left = left_;
    ScalarPtr right = right_;
    if (operation_.isCommutative()) {
      const bool self_order = (left_->hash() < right_->hash());
      const bool other_order = (expr->left_->hash() < expr->right_->hash());
      if (self_order ^ other_order) {
        std::swap(left, right);
      }
    }
    return left->equals(expr->left_) && right->equals(expr->right_);
  }
  return false;
}

void BinaryExpression::getFieldStringItems(
    std::vector<std::string> *inline_field_names,
    std::vector<std::string> *inline_field_values,
    std::vector<std::string> *non_container_child_field_names,
    std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
    std::vector<std::string> *container_child_field_names,
    std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const {
  container_child_field_names->push_back("");
  container_child_fields->push_back({left_, right_});
}

}  // namespace expressions
}  // namespace optimizer
}  // namespace quickstep
