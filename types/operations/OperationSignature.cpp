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

#include "types/operations/OperationSignature.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "types/CastUtil.hpp"
#include "types/Type.hpp"
#include "types/TypeFactory.hpp"
#include "types/TypedValue.hpp"
#include "types/operations/OperationSignatureLite.hpp"
#include "utility/HashPair.hpp"

#include "glog/logging.h"

namespace quickstep {

serialization::OperationSignature OperationSignature::getProto() const {
  serialization::OperationSignature proto;
  proto.mutable_sig_lite()->MergeFrom(sig_lite_->getProto());
  for (const Type *type : param_types_) {
    proto.add_param_types()->MergeFrom(type->getProto());
  }
  for (const TypedValue &value : static_param_values_) {
    proto.add_static_param_values()->MergeFrom(value.getProto());
  }
  return proto;
}

OperationSignaturePtr OperationSignature::ReconstructFromProto(
    const serialization::OperationSignature &proto) {
  DCHECK(ProtoIsValid(proto))
      << "Attempted to create OperationSignature from an invalid proto "
      << "description:\n" << proto.DebugString();

  const OperationSignatureLitePtr sig_lite =
      OperationSignatureLite::ReconstructFromProto(proto.sig_lite());

  std::vector<const Type*> param_types;
  std::vector<TypedValue> static_param_values;

  const int num_params = proto.param_types_size();
  DCHECK_EQ(num_params, proto.static_param_values_size());
  param_types.reserve(num_params);
  static_param_values.reserve(num_params);

  for (int i = 0; i < num_params; ++i) {
    param_types.emplace_back(
        &TypeFactory::ReconstructFromProto(proto.param_types(i)));
    static_param_values.emplace_back(
        TypedValue::ReconstructFromProto(proto.static_param_values(i)));
  }
  return OperationSignature::Create(sig_lite, param_types, static_param_values);
}

bool OperationSignature::ProtoIsValid(
    const serialization::OperationSignature &proto) {
  if (!OperationSignatureLite::ProtoIsValid(proto.sig_lite())) {
    return false;
  }
  const int num_params = proto.param_types_size();
  if (num_params != proto.static_param_values_size()) {
    return false;
  }
  for (int i = 0; i < num_params; ++i) {
    if (!TypeFactory::ProtoIsValid(proto.param_types(i))) {
      return false;
    }
    if (!TypedValue::ProtoIsValid(proto.static_param_values(i))) {
      return false;
    }
  }
  return true;
}

bool OperationSignature::operator==(const OperationSignature &other) const {
  if (*sig_lite_ != *other.sig_lite_) {
    return false;
  }
  DCHECK_EQ(getNumParameters(), other.getNumParameters());
  for (std::size_t i = 0; i < static_param_values_.size(); ++i) {
    if (!isStaticParameter(i)) {
      continue;
    }
    DCHECK(other.isStaticParameter(i));
    if (!static_param_values_[i].equals(other.static_param_values_[i])) {
      return false;
    }
  }
  return true;
}

OperationSignaturePtr OperationSignature::getCoercedSignature(
    const OperationSignatureLitePtr &target_signature) const {
  DCHECK(sig_lite_->getCoercionPrecedence(target_signature).isValid());

  const std::size_t num_params = getNumParameters();
  DCHECK_EQ(num_params, target_signature->getNumParameters());

  std::vector<const Type*> coerced_types;
  std::vector<TypedValue> coerced_values;
  coerced_types.reserve(num_params);
  coerced_values.reserve(num_params);

  for (std::size_t i = 0; i < num_params; ++i) {
    const Type &source_type = getParameterType(i);
    const TypeID target_type_id = target_signature->getParameterTypeID(i);

    // Get coerced type.
    const Type &target_type = CastUtil::CoerceType(source_type, target_type_id);
    coerced_types.emplace_back(&target_type);

    if (!target_signature->isStaticParameter(i)) {
      coerced_values.emplace_back();
      continue;
    }
    DCHECK(isStaticParameter(i));

    // Get coerced static argument.
    const TypedValue &source_value = getStaticParameterValue(i);
    coerced_values.emplace_back(target_type.coerceValue(source_value, source_type));
  }
  return OperationSignature::Create(target_signature, coerced_types, coerced_values);
}

std::size_t OperationSignature::getHash() const {
  std::size_t hash_code = sig_lite_->getHash();
  for (std::size_t i = 0; i < param_types_.size(); ++i) {
    if (!isStaticParameter(i)) {
      hash_code = CombineHashes(hash_code, static_param_values_[i].getHash());
    }
  }
  return hash_code;
}

std::string OperationSignature::toString() const {
  std::string str;
  str.append(sig_lite_->getOperationName());
  str.push_back('(');
  for (std::size_t i = 0; i < param_types_.size(); ++i) {
    if (i != 0) {
      str.append(", ");
    }
    const Type &param_type = *param_types_[i];
    if (isStaticParameter(i)) {
      str.append(param_type.printValueToString(static_param_values_[i]));
    } else {
      str.append(param_type.getName());
    }
  }
  str.push_back(')');
  return str;
}

OperationSignaturePtr OperationSignature::Create(
    const OperationSignatureLitePtr &sig_lite,
    const std::vector<const Type*> &param_types) {
  return Create(sig_lite, param_types,
                std::vector<TypedValue>(param_types.size()));
}

OperationSignaturePtr OperationSignature::Create(
    const OperationSignatureLitePtr &sig_lite,
    const std::vector<const Type*> &param_types,
    const std::vector<TypedValue> &static_param_values) {
  return OperationSignaturePtr(
      new OperationSignature(sig_lite, param_types, static_param_values));
}

}  // namespace quickstep
