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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_CAST_OPERATION_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_CAST_OPERATION_HPP_

#include <string>
#include <vector>

#include "types/Type.hpp"
#include "types/TypedValue.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperationSignatureLite.hpp"
#include "types/operations/unary_operations/UnaryOperation.hpp"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

/**
 * @brief Type casting operation.
 */
class CastOperation : public UnaryOperation {
 public:
  /**
   * @brief Constructor.
   */
  CastOperation() {}

  std::string getName() const override {
    return "Cast";
  }

  std::string getShortName() const override {
    return "Cast";
  }

  std::vector<OperationSignatureLitePtr> getSignatures() const override;

  std::size_t getOperatorPrecedence() const override;

  bool canApplyToSignature(const OperationSignaturePtr &signature,
                           std::string *diagnostic_message) const override;

  const Type& resultTypeForSignature(
      const OperationSignaturePtr &signature) const override;

  UncheckedUnaryOperator* makeUncheckedUnaryOperatorForSignature(
      const OperationSignaturePtr &signature) const override;

  std::string formatExpression(const OperationSignaturePtr &signature,
                               const std::string &argument,
                               const std::size_t argument_precedence) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastOperation);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_CAST_OPERATION_HPP_
