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

#include "types/TypeFactory.hpp"

#include <cstddef>
#include <exception>
#include <regex>
#include <string>
#include <unordered_map>

#include "types/CharType.hpp"
#include "types/DateType.hpp"
#include "types/DatetimeIntervalType.hpp"
#include "types/DatetimeType.hpp"
#include "types/DoubleType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/NullType.hpp"
#include "types/Type.hpp"
#include "types/Type.pb.h"
#include "types/TypeID.hpp"
#include "types/VarCharType.hpp"
#include "types/YearMonthIntervalType.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

const Type& TypeFactory::GetType(const TypeID id,
                                 const bool nullable) {
  switch (id) {
    case kInt:
      return IntType::Instance(nullable);
    case kLong:
      return LongType::Instance(nullable);
    case kFloat:
      return FloatType::Instance(nullable);
    case kDouble:
      return DoubleType::Instance(nullable);
    case kDate:
      return DateType::Instance(nullable);
    case kDatetime:
      return DatetimeType::Instance(nullable);
    case kDatetimeInterval:
      return DatetimeIntervalType::Instance(nullable);
    case kYearMonthInterval:
      return YearMonthIntervalType::Instance(nullable);
    case kNullType:
      DCHECK(nullable);
      return NullType::InstanceNullable();
    default:
      FATAL_ERROR("Called TypeFactory::GetType() for a type which requires "
                  " a length parameter without specifying one.");
  }
}

const Type& TypeFactory::GetType(const TypeID id,
                                 const std::size_t length,
                                 const bool nullable) {
  switch (id) {
    case kChar:
      return CharType::Instance(length, nullable);
    case kVarChar:
      return VarCharType::Instance(length, nullable);
    default:
      FATAL_ERROR("Provided a length parameter to TypeFactory::GetType() for "
                  "a type which does not take one.");
  }
}

bool TypeFactory::ProtoIsValid(const serialization::Type &proto) {
  // Check that proto is fully initialized.
  if (!proto.IsInitialized()) {
    return false;
  }

  // Check that the type_id is valid, and extensions if any.
  const TypeID type_id = ReconstructTypeIDFromProto(proto.type_id());
  switch (type_id) {
    case kInt:
    case kLong:
    case kFloat:
    case kDouble:
    case kDate:
    case kDatetime:
    case kDatetimeInterval:
    case kYearMonthInterval:
      return true;
    case kChar:
      return proto.HasExtension(serialization::CharType::length);
    case kVarChar:
      return proto.HasExtension(serialization::VarCharType::length);
    case kNullType:
      return proto.nullable();
    default:
      return false;
  }
}

const Type& TypeFactory::ReconstructFromProto(const serialization::Type &proto) {
  DCHECK(ProtoIsValid(proto))
      << "Attempted to create Type from an invalid proto description:\n"
      << proto.DebugString();

  const TypeID type_id = ReconstructTypeIDFromProto(proto.type_id());
  switch (type_id) {
    case kInt:
      return IntType::Instance(proto.nullable());
    case kLong:
      return LongType::Instance(proto.nullable());
    case kFloat:
      return FloatType::Instance(proto.nullable());
    case kDouble:
      return DoubleType::Instance(proto.nullable());
    case kDate:
      return DateType::Instance(proto.nullable());
    case kDatetime:
      return DatetimeType::Instance(proto.nullable());
    case kDatetimeInterval:
      return DatetimeIntervalType::Instance(proto.nullable());
    case kYearMonthInterval:
      return YearMonthIntervalType::Instance(proto.nullable());
    case kChar:
      return CharType::InstanceFromProto(proto);
    case kVarChar:
      return VarCharType::InstanceFromProto(proto);
    case kNullType:
      DCHECK(proto.nullable());
      return NullType::InstanceNullable();
    default:
      FATAL_ERROR("Unrecognized TypeID in TypeFactory::ReconstructFromProto");
  }
}

const Type* TypeFactory::GetMostSpecificType(const Type &first, const Type &second) {
  if (first.isSafelyCoercibleFrom(second)) {
    return &first;
  } else if (second.isSafelyCoercibleFrom(first)) {
    return &second;
  } else {
    return nullptr;
  }
}

const Type* TypeFactory::GetUnifyingType(const Type &first, const Type &second) {
  const Type *unifier = nullptr;
  if (first.isNullable() || second.isNullable()) {
    unifier = GetMostSpecificType(first.getNullableVersion(), second.getNullableVersion());
    if (unifier == nullptr) {
      if (((first.getTypeID() == kLong) && (second.getTypeID() == kFloat))
            || ((first.getTypeID() == kFloat) && (second.getTypeID() == kLong))) {
        unifier = &(DoubleType::Instance(true));
      }
    }
  } else {
    unifier = GetMostSpecificType(first, second);
    if (unifier == nullptr) {
      if (((first.getTypeID() == kLong) && (second.getTypeID() == kFloat))
            || ((first.getTypeID() == kFloat) && (second.getTypeID() == kLong))) {
        unifier = &(DoubleType::Instance(false));
      }
    }
  }

  return unifier;
}

const Type* TypeFactory::ParseTypeFromString(const std::string &type_name) {
  // TODO(Jianqiao): We will use a systematical approach to parse types which is
  // similar to operation resolution in subsequent type refactoring commits.
  static const std::regex type_regex("([a-zA-Z]+)(\\(([0-9]+)\\))?( NULL)?");
  static const std::unordered_map<std::string, TypeID> type_index =
      { { "Int", kInt }, { "Long", kLong },
        { "Float", kFloat }, { "Double", kDouble },
        { "Char", kChar }, { "VarChar", kVarChar },
        { "Date", kDate }, { "Datetime", kDatetime },
        { "DatetimeInterval", kDatetimeInterval },
        { "YearMonthInterval", kYearMonthInterval } };

  std::smatch match;
  if (!std::regex_match(type_name, match, type_regex)) {
    return nullptr;
  }

  const auto index_it = type_index.find(match.str(1));
  if (index_it == type_index.end()) {
    return nullptr;
  }

  const TypeID type_id = index_it->second;
  const bool nullable = !match.str(4).empty();

  switch (type_id) {
    case kInt:  // Fall through
    case kLong:
    case kFloat:
    case kDouble:
    case kDate:
    case kDatetime:
    case kDatetimeInterval:
    case kYearMonthInterval: {
      return &GetType(type_id, nullable);
    }
    case kChar:  // Fall through
    case kVarChar: {
      std::size_t length;
      try {
        length = std::stoull(match.str(3));
      } catch (const std::exception &e) {
        return nullptr;
      }
      return &GetType(type_id, length, nullable);
    }
    default:
      break;
  }
  return nullptr;
}

}  // namespace quickstep
