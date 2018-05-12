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

#include "types/operations/binary_operations/BinaryOperation.hpp"

#include <cstddef>
#include <string>

#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperatorPrecedence.hpp"

namespace quickstep {

std::string BinaryOperation::formatExpression(const OperationSignaturePtr &signature,
                                              const std::string &left,
                                              const std::size_t left_precedence,
                                              const std::string &right,
                                              const std::size_t right_precedence) const {
  const std::size_t precedence = getOperatorPrecedence();
  if (precedence != kOperatorPrecedenceFunctionCall) {
    std::string expression;
    expression.append(precedence <= left_precedence ? "(" + left + ")" : left);
    expression.append(getShortName());
    expression.append(precedence <= right_precedence ? "(" + right + ")" : right);
    return expression;
  }
  return getName() + "(" + left + "," + right + ")";
}

}  // namespace quickstep
