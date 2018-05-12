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

#include "types/operations/OperationFactory.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "types/operations/Operation.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperationSignatureCoercionPrecedence.hpp"
#include "types/operations/OperationSignatureLite.hpp"
#include "types/operations/unary_operations/ArithmeticUnaryOperations.hpp"
#include "types/operations/unary_operations/CastOperation.hpp"
#include "types/operations/unary_operations/UnaryOperation.hpp"
#include "types/operations/unary_operations/UnaryOperationCodeGen.hpp"

#include "glog/logging.h"

namespace quickstep {

const OperationFactory& OperationFactory::Instance() {
  static OperationFactory instance;
  return instance;
}

OperationFactory::OperationFactory() {
  // Register individual operations.
  registerOperation<CastOperation>();

  // Register functor lists.
  registerFunctorsCodeGen<ArithmeticUnaryFunctors>();
}

bool OperationFactory::HasOperation(const OperationSignatureLitePtr &signature) {
  const auto &operations = Instance().operations_;
  const auto it = operations.find(signature);
  return it != operations.end();
}

bool OperationFactory::HasOperation(const std::string &op_name,
                                    const std::size_t arity) {
  const auto &name_arity_index = Instance().name_arity_index_;
  const auto it = name_arity_index.find(std::make_pair(op_name, arity));
  return it != name_arity_index.end();
}

const Operation& OperationFactory::GetOperation(
    const OperationSignatureLitePtr &signature) {
  DCHECK(OperationFactory::HasOperation(signature));
  const OperationPtr &operation = Instance().operations_.at(signature);
  DCHECK(operation != nullptr);
  return *operation;
}

const UnaryOperation& OperationFactory::GetUnaryOperation(
    const OperationSignatureLitePtr &signature) {
  return static_cast<const UnaryOperation&>(GetOperation(signature));
}

OperationSignaturePtr OperationFactory::ResolveSignature(
    const OperationSignaturePtr &signature,
    std::string *diagnostic_message) {
  return Instance().resolveInternal(signature, diagnostic_message);
}

template <typename ...FunctorClasses>
void OperationFactory::registerFunctorsCodeGen() {
  meta::TypeList<FunctorClasses...>
      ::template flatten<>
      ::ForEach([&](auto typelist) -> void {
    using FunctorClass = typename decltype(typelist)::head;
    registerOperation<typename FunctorClass::template OperationCodeGen<FunctorClass>>();
  });
}

void OperationFactory::registerOperation(const OperationPtr &operation) {
  for (const auto &signature : operation->getSignatures()) {
    const auto ret = operations_.emplace(signature, operation);
    CHECK(ret.second) << "Duplicated registration of operation signature "
                      << signature->toString();
    name_arity_index_[std::make_pair(signature->getOperationName(),
                                     signature->getNumParameters())]
        .emplace_back(signature);
  }
}

template <typename OperationClass>
void OperationFactory::registerOperation() {
  registerOperation(std::make_shared<OperationClass>());
}

OperationSignaturePtr OperationFactory::resolveInternal(
    const OperationSignaturePtr &signature,
    std::string *diagnostic_message) const {
  ResolveStatus status;
  OperationSignaturePtr resolved_signature;

  // First try exact matching.
  status = resolveExactMatch(signature, &resolved_signature, diagnostic_message);
  if (status == ResolveStatus::kSuccess) {
    DCHECK(resolved_signature != nullptr);
    return resolved_signature;
  } else if (status == ResolveStatus::kError) {
    return nullptr;
  }

  // Otherwise try partial matching with implicit type coersion.
  status = resolvePartialMatch(signature, &resolved_signature, diagnostic_message);
  if (status == ResolveStatus::kSuccess) {
    DCHECK(resolved_signature != nullptr);
    return resolved_signature;
  } else if (status == ResolveStatus::kError) {
    return nullptr;
  }

  // TODO(jianqiao): More informative error message.
  const auto &op_name = signature->getOperationName();
  *diagnostic_message = "Unexpected arguments for function \"" + op_name + "\"";
  return nullptr;
}

OperationFactory::ResolveStatus OperationFactory::resolveExactMatch(
    const OperationSignaturePtr &signature,
    OperationSignaturePtr *resolved,
    std::string *diagnostic_message) const {
  const auto it = operations_.find(signature->getSignatureLite());
  if (it == operations_.end()) {
    return ResolveStatus::kNotFound;
  }
  if (it->second->canApplyToSignature(signature, diagnostic_message)) {
    *resolved = signature;
    return ResolveStatus::kSuccess;
  }
  return ResolveStatus::kError;
}

OperationFactory::ResolveStatus OperationFactory::resolvePartialMatch(
    const OperationSignaturePtr &signature,
    OperationSignaturePtr *resolved,
    std::string *diagnostic_message) const {
  const auto name_arity = std::make_pair(signature->getOperationName(),
                                         signature->getNumParameters());
  const auto it = name_arity_index_.find(name_arity);
  if (it == name_arity_index_.end()) {
    return OperationFactory::ResolveStatus::kNotFound;
  }

  // Get all candidate signatures with partial match.
  using OSCP = OperationSignatureCoercionPrecedence;
  using CandidateInfo = std::pair<OSCP, const OperationSignatureLitePtr*>;
  std::vector<CandidateInfo> candidates;
  for (const auto &target_signature : it->second) {
    const OSCP precedence = signature->getSignatureLite()
                                     ->getCoercionPrecedence(target_signature);
    if (precedence.isValid()) {
      candidates.emplace_back(std::make_pair(precedence, &target_signature));
    }
  }

  if (candidates.empty()) {
    return OperationFactory::ResolveStatus::kNotFound;
  }

  std::sort(candidates.begin(), candidates.end(),
            [](const auto &lhs, const auto &rhs) -> bool {
              return rhs.first < lhs.first;
            });

  const CandidateInfo *best = &candidates[0];
  // Check whether overload is ambiguous.
  for (std::size_t i = 1; i < candidates.size(); ++i) {
    const CandidateInfo &curr = candidates[i];
    if (best->first != curr.first) {
      // We are certain that the rest candidates do not equally best-match the
      // input signature.
      break;
    }
    const bool best_is_more_specialized =
        (*best->second)->getCoercionPrecedence(*curr.second).isValid();
    const bool curr_is_more_specialized =
        (*curr.second)->getCoercionPrecedence(*best->second).isValid();
    if (best_is_more_specialized == curr_is_more_specialized) {
      // No one signature is strictly more specialized than the other.
      best = nullptr;
      break;
    }
    if (curr_is_more_specialized) {
      best = &curr;
    }
  }

  if (best == nullptr) {
    // There are at least two signatures that equally best-match the input
    // signature. Report this ambiguous overload error.
    *diagnostic_message = "Ambiguous overload for operation \"" +
                          name_arity.first + "\".\nCandidates are: ";
    const auto &best_match = candidates[0].first;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      if (candidates[i].first != best_match) {
        break;
      }
      if (i != 0) {
        diagnostic_message->append("                ");
      }
      diagnostic_message->append((*candidates[i].second)->toString());
      diagnostic_message->push_back('\n');
    }
    return OperationFactory::ResolveStatus::kError;
  }

  *resolved = signature->getCoercedSignature(*candidates[0].second);
  return OperationFactory::ResolveStatus::kSuccess;
}

}  // namespace quickstep
