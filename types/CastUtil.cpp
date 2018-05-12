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

#include "types/CastUtil.hpp"

#include <cstddef>
#include <limits>

#include "storage/StorageConstants.hpp"
#include "types/Type.hpp"
#include "types/TypeFactory.hpp"
#include "types/TypeID.hpp"

#include "glog/logging.h"

namespace quickstep {

namespace {

const Type& GetCanonicalTypeInstance(const TypeID type_id, const bool nullable) {
  switch (type_id) {
    case kInt:  // Fall through
    case kLong:
    case kFloat:
    case kDouble:
    case kDate:
    case kDatetime:
    case kDatetimeInterval:
    case kYearMonthInterval:
      return TypeFactory::GetType(type_id, nullable);
    case kChar:  // Fall through
    case kVarChar:
      return TypeFactory::GetType(type_id, kSlotSizeBytes, nullable);
    default:
      LOG(FATAL) << "Unrecognized TypeID in GetCanonicalInstance()";
  }
}

}  // namespace

bool CastUtil::IsSafelyCoercible(const TypeID source_type_id,
                                 const TypeID target_type_id) {
  const Type &source_type = GetCanonicalTypeInstance(source_type_id, true);
  const Type &target_type = GetCanonicalTypeInstance(target_type_id, true);
  return target_type.isSafelyCoercibleFrom(source_type);
}

const Type& CastUtil::CoerceType(const Type &source_type,
                                 const TypeID target_type_id) {
  const Type &target_type =
      GetCanonicalTypeInstance(target_type_id, source_type.isNullable());
  switch (target_type.getSuperTypeID()) {
    case Type::kNumeric:  // Fall through
    case Type::kOther: {
      return target_type;
    }
    case Type::kAsciiString: {
      CHECK(source_type.getSuperTypeID() == Type::kAsciiString);
      const AsciiStringSuperType &ascii_string_source_type =
          static_cast<const AsciiStringSuperType&>(source_type);
      return TypeFactory::GetType(target_type_id,
                                  ascii_string_source_type.getStringLength(),
                                  source_type.isNullable());
    }
    default:
      LOG(FATAL) << "Unrecognized Type::SuperTypeID in CoerceType()";
  }
}

}  // namespace quickstep
