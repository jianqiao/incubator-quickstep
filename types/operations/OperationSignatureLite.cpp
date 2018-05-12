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

#include "types/operations/OperationSignatureLite.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "types/CastUtil.hpp"
#include "types/TypeID.hpp"
#include "types/operations/Operation.pb.h"
#include "types/operations/OperationSignatureCoercionPrecedence.hpp"
#include "utility/HashPair.hpp"

#include "glog/logging.h"

namespace quickstep {

serialization::OperationSignatureLite OperationSignatureLite::getProto() const {
  serialization::OperationSignatureLite proto;
  proto.set_op_name(op_name_);
  for (const TypeID type_id : param_type_ids_) {
    proto.add_param_type_ids()->MergeFrom(GetTypeIDProto(type_id));
  }
  for (const bool value : param_is_static_) {
    proto.add_param_is_static(value);
  }
  return proto;
}

OperationSignatureLitePtr OperationSignatureLite::ReconstructFromProto(
    const serialization::OperationSignatureLite &proto) {
  DCHECK(ProtoIsValid(proto))
      << "Attempted to create OperationSignatureLite from an invalid proto "
      << "description:\n" << proto.DebugString();

  std::vector<TypeID> param_type_ids;
  std::vector<bool> param_is_static;

  const int num_params = proto.param_type_ids_size();
  DCHECK_EQ(num_params, proto.param_is_static_size());
  param_type_ids.reserve(num_params);
  param_is_static.reserve(num_params);

  for (int i = 0; i < num_params; ++i) {
    param_type_ids.emplace_back(ReconstructTypeIDFromProto(proto.param_type_ids(i)));
    param_is_static.emplace_back(proto.param_is_static(i));
  }
  return OperationSignatureLite::Create(
      proto.op_name(), param_type_ids, param_is_static);
}

bool OperationSignatureLite::ProtoIsValid(
    const serialization::OperationSignatureLite &proto) {
  if (!proto.has_op_name()) {
    return false;
  }
  const int num_params = proto.param_type_ids_size();
  if (num_params != proto.param_is_static_size()) {
    return false;
  }
  for (int i = 0; i < num_params; ++i) {
    if (!TypeIDProtoIsValid(proto.param_type_ids(i))) {
      return false;
    }
  }
  return true;
}

OperationSignatureCoercionPrecedence OperationSignatureLite::getCoercionPrecedence(
    const OperationSignatureLitePtr &target_signature) const {
  using OSCP = OperationSignatureCoercionPrecedence;

  // Coercion is invalid if number of arguments mismatch.
  if (getNumParameters() != target_signature->getNumParameters()) {
    return OSCP();
  }

  // 0 -- Non-static arguments coercibility level.
  // 1 -- Static arguments coercibility level.
  std::array<OSCP::CoercibilityLevel, 2> coercibility_levels =
      {{ OSCP::CoercibilityLevel::kExactMatch, OSCP::CoercibilityLevel::kExactMatch }};

  for (std::size_t i = 0; i < target_signature->getNumParameters(); ++i) {
    const bool target_is_static = target_signature->isStaticParameter(i);
    if (!isStaticParameter(i) && target_is_static) {
      // Cannot cast a non-static argument to a static argument.
      return OSCP();
    }

    OSCP::CoercibilityLevel *level = &coercibility_levels[target_is_static];
    const TypeID source_type_id = getParameterTypeID(i);
    const TypeID target_type_id = target_signature->getParameterTypeID(i);

    // Case 1. Exact match.
    if (source_type_id == target_type_id) {
      continue;
    }
    // Case 2. Safely coercible.
    if (CastUtil::IsSafelyCoercible(source_type_id, target_type_id)) {
      *level = OSCP::GetUnifyingCoercibilityLevel(*level, OSCP::kSafelyCoercible);
      continue;
    }
    // Case 3. Not safe but coercible -- we disallow implicit type casting in this case.

    // There is at least one argument that is not safely coercible to the target type.
    return OSCP();
  }

  return OSCP(coercibility_levels[0],
              coercibility_levels[1],
              target_signature->getNumStaticParameters());
}

std::size_t OperationSignatureLite::getHash() const {
  std::size_t hash_code = CombineHashes(std::hash<std::string>()(op_name_),
                                        param_type_ids_.size());
  for (const TypeID tid : param_type_ids_) {
    hash_code = CombineHashes(hash_code, tid);
  }
  for (const bool val : param_is_static_) {
    hash_code = CombineHashes(hash_code, val);
  }
  return hash_code;
}

std::string OperationSignatureLite::toString() const {
  std::string str;
  str.append(op_name_);
  str.push_back('(');
  for (std::size_t i = 0; i < param_type_ids_.size(); ++i) {
    if (i != 0) {
      str.append(", ");
    }
    const std::string &type_name = kTypeNames[param_type_ids_[i]];
    if (param_is_static_[i]) {
      str.append("static(" + type_name + ")");
    } else {
      str.append(type_name);
    }
  }
  str.push_back(')');
  return str;
}

OperationSignatureLitePtr OperationSignatureLite::Create(
    const std::string &op_name,
    const std::vector<TypeID> &param_type_ids) {
  return Create(op_name, param_type_ids,
                std::vector<bool>(param_type_ids.size(), false));
}

OperationSignatureLitePtr OperationSignatureLite::Create(
    const std::string &op_name,
    const std::vector<TypeID> &param_type_ids,
    const std::vector<bool> &param_is_static) {
  return OperationSignatureLitePtr(
      new OperationSignatureLite(op_name, param_type_ids, param_is_static));
}

std::size_t OperationSignatureLite::CountNumStaticParameters(
    const std::vector<bool> &param_is_static) {
  std::size_t num_static_params = 0;
  for (const bool value : param_is_static) {
    num_static_params += value;
  }
  return num_static_params;
}

}  // namespace quickstep
