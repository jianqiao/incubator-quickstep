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

#include "types/Type.hpp"

#include "types/Type.pb.h"
#include "types/TypeID.hpp"
#include "types/TypedValue.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

serialization::Type Type::getProto() const {
  serialization::Type proto;
  proto.mutable_type_id()->MergeFrom(GetTypeIDProto(type_id_));
  proto.set_nullable(nullable_);
  return proto;
}

TypedValue Type::coerceValue(const TypedValue &original_value,
                             const Type &original_type) const {
  DCHECK(isCoercibleFrom(original_type))
      << "Can't coerce value of Type " << original_type.getName()
      << " to Type " << getName();

  if (original_type.getTypeID() == kNullType) {
    return makeNullValue();
  }

  DCHECK(equals(original_type) || equals(original_type.getNullableVersion()))
      << "Base version of Type::coerceValue() called for a non-trivial "
      << "coercion from Type " << original_type.getName()
      << " to Type " << getName();

  return original_value;
}

bool AsciiStringSuperType::isCoercibleFrom(const Type &original_type) const {
  if (original_type.isNullable() && !nullable_) {
    return false;
  }
  return (original_type.getSuperTypeID() == kAsciiString)
         || (original_type.getTypeID() == kNullType);
}

}  // namespace quickstep
