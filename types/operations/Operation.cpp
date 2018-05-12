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

#include "types/operations/Operation.hpp"

#include <string>

#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperatorPrecedence.hpp"

namespace quickstep {

const char *Operation::kOperationSuperTypeNames[] = {
  "Comparison",
  "UnaryOperation",
  "BinaryOperation"
};

std::vector<OperationSignatureLitePtr> Operation::getSignatures() const {
  LOG(FATAL) << "Not implemented";
}

std::size_t Operation::getOperatorPrecedence() const {
  return kOperatorPrecedenceFunctionCall;
}

bool Operation::canApplyToSignature(const OperationSignaturePtr &signature,
                                    std::string *diagnostic_message) const {
  LOG(FATAL) << "Not implemented";
}

bool Operation::canApplyToSignature(const OperationSignaturePtr &signature) const {
  std::string dumb_message;
  return canApplyToSignature(signature, &dumb_message);
}

const Type& Operation::resultTypeForSignature(
    const OperationSignaturePtr &signature) const {
  LOG(FATAL) << "Not implemented";
}

}  // namespace quickstep
