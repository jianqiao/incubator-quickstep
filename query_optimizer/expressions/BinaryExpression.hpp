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

#ifndef QUICKSTEP_QUERY_OPTIMIZER_EXPRESSIONS_BINARY_EXPRESSION_HPP_
#define QUICKSTEP_QUERY_OPTIMIZER_EXPRESSIONS_BINARY_EXPRESSION_HPP_

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/expressions/ExprId.hpp"
#include "query_optimizer/expressions/Expression.hpp"
#include "query_optimizer/expressions/ExpressionType.hpp"
#include "query_optimizer/expressions/Scalar.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/binary_operations/BinaryOperation.hpp"
#include "utility/Macros.hpp"

namespace quickstep {

class BinaryOperation;
class CatalogAttribute;
class Type;

namespace optimizer {
namespace expressions {

/** \addtogroup OptimizerExpressions
 *  @{
 */

class BinaryExpression;
typedef std::shared_ptr<const BinaryExpression> BinaryExpressionPtr;

/**
 * @brief Applies a binary operator to two scalar expressions and returns a
 *        scalar value.
 */
class BinaryExpression : public Scalar {
 public:
  ExpressionType getExpressionType() const override {
    return ExpressionType::kBinaryExpression;
  }

  std::string getName() const override;

  const Type& getValueType() const override {
    return result_type_;
  }

  bool isConstant() const override {
    return left_->isConstant() && right_->isConstant();
  }

  /**
   * @return The query-time signature of the binary operation.
   */
  const OperationSignaturePtr signature() const { return signature_; }

  /**
   * @return The binary operation.
   */
  const BinaryOperation& operation() const { return operation_; }

  /**
   * @return The left operand.
   */
  const ScalarPtr& left() const { return left_; }

  /**
   * @return The right operand.
   */
  const ScalarPtr& right() const { return right_; }

  ExpressionPtr copyWithNewChildren(
      const std::vector<ExpressionPtr> &new_children) const override;

  std::vector<AttributeReferencePtr> getReferencedAttributes() const override;

  ::quickstep::Scalar* concretize(
      const std::unordered_map<ExprId, const CatalogAttribute*> &substitution_map,
      const std::unordered_set<ExprId> &left_expr_ids = std::unordered_set<ExprId>(),
      const std::unordered_set<ExprId> &right_expr_ids = std::unordered_set<ExprId>()) const override;

  bool equals(const ScalarPtr &other) const override;

  std::pair<std::string, std::size_t> generateNameWithPrecedence() const override;

  /**
   * @brief Creates an immutable BinaryExpression.
   *
   * @param signature The query-time signature of the operation.
   * @param operation The binary operation.
   * @param left The left operand.
   * @param right The right operand.
   * @return An immutable BinaryExpression that applies the operation to the
   *         operands.
   */
  static BinaryExpressionPtr Create(const OperationSignaturePtr &signature,
                                    const BinaryOperation &operation,
                                    const ScalarPtr &left,
                                    const ScalarPtr &right) {
    return BinaryExpressionPtr(new BinaryExpression(signature, operation, left, right));
  }

 protected:
  std::size_t computeHash() const override;

  void getFieldStringItems(
      std::vector<std::string> *inline_field_names,
      std::vector<std::string> *inline_field_values,
      std::vector<std::string> *non_container_child_field_names,
      std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
      std::vector<std::string> *container_child_field_names,
      std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const override;

 private:
  BinaryExpression(const OperationSignaturePtr &signature,
                   const BinaryOperation &operation,
                   const ScalarPtr &left,
                   const ScalarPtr &right);

  const OperationSignaturePtr signature_;
  const BinaryOperation &operation_;
  const ScalarPtr left_;
  const ScalarPtr right_;
  const Type &result_type_;

  DISALLOW_COPY_AND_ASSIGN(BinaryExpression);
};

/** @} */

}  // namespace expressions
}  // namespace optimizer
}  // namespace quickstep

#endif /* QUICKSTEP_QUERY_OPTIMIZER_EXPRESSIONS_BINARY_EXPRESSION_HPP_ */
