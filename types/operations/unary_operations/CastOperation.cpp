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

#include "types/operations/unary_operations/CastOperation.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>
#include <string>

#include "types/CharType.hpp"
#include "types/DateType.hpp"
#include "types/DatetimeType.hpp"
#include "types/DatetimeIntervalType.hpp"
#include "types/DoubleType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/Type.hpp"
#include "types/TypeFactory.hpp"
#include "types/TypeID.hpp"
#include "types/TypeTraits.hpp"
#include "types/TypedValue.hpp"
#include "types/VarCharType.hpp"
#include "types/YearMonthIntervalType.hpp"
#include "types/operations/unary_operations/UnaryFunctor.hpp"
#include "types/operations/unary_operations/UnaryOperationCodeGen.hpp"
#include "utility/EqualsAnyConstant.hpp"
#include "utility/meta/Common.hpp"
#include "utility/meta/MultipleDispatcher.hpp"
#include "utility/meta/StringConstant.hpp"
#include "utility/meta/TypeList.hpp"

#include "glog/logging.h"

namespace quickstep {

namespace {

// Cast a numeric type to another numeric type.
template <typename SourceType, typename TargetType>
struct CastNumericToNumericFunctor : public UnaryFunctor<STR_CONST8("Cast"),
                                                         STR_CONST8("Cast"),
                                                         SourceType,
                                                         TargetType> {
  using SourceCppType = typename SourceType::cpptype;
  using TargetCppType = typename TargetType::cpptype;
  inline void operator()(const SourceCppType *argument,
                         TargetCppType *result) const {
    *result = static_cast<TargetCppType>(*argument);
  }
};

// Cast any type to char or varchar type.
// TODO(Jianqiao): Improve efficiency.
template <typename SourceType>
class CastAnyToCharFunctor : public UnaryFunctor<STR_CONST8("Cast"),
                                                 STR_CONST8("Cast"),
                                                 SourceType,
                                                 CharType> {
 public:
  CastAnyToCharFunctor(const SourceType &source_type,
                       const CharType &target_type)
      : source_type_(source_type),
        target_type_(target_type),
        max_result_length_(target_type.getStringLength()) {}

  using SourceCppType = typename SourceType::cpptype;

  // CxxInlinePod -> Char
  inline void operator()(const SourceCppType *argument, void *result) const {
    const std::string value =
        source_type_.printValueToString(TypedValue(*argument));
    const std::size_t length = value.length();
    if (length < max_result_length_) {
      std::memcpy(result, value.c_str(), length);
      static_cast<char *>(result)[length] = 0;
    } else {
      std::memcpy(result, value.c_str(), max_result_length_);
    }
  }

 private:
  const SourceType &source_type_;
  const CharType &target_type_;
  const std::size_t max_result_length_;
};

template <typename SourceType>
class CastAnyToVarCharFunctor : public UnaryFunctor<STR_CONST8("Cast"),
                                                    STR_CONST8("Cast"),
                                                    SourceType,
                                                    VarCharType> {
 public:
  CastAnyToVarCharFunctor(const SourceType &source_type,
                          const VarCharType &target_type)
      : source_type_(source_type),
        target_type_(target_type),
        max_result_length_(target_type.getStringLength()) {}

  using SourceCppType = typename SourceType::cpptype;

  // CxxInlinePod -> VarChar
  inline void operator()(const SourceCppType *argument, TypedValue *result) const {
    const std::string value =
        source_type_.printValueToString(TypedValue(*argument));
    const std::size_t actual_length = std::min(value.length(), max_result_length_);
    const std::size_t size = actual_length + 1;
    char *data = static_cast<char*>(std::malloc(size));
    std::memcpy(data, value.c_str(), actual_length);
    data[actual_length] = 0;
    *result = TypedValue::CreateWithOwnedData(target_type_.getTypeID(), data, size);
  }

 private:
  const SourceType &source_type_;
  const VarCharType &target_type_;
  const std::size_t max_result_length_;
};

}  // namespace

std::vector<OperationSignatureLitePtr> CastOperation::getSignatures() const {
  const std::vector<TypeID> tids =
      { kInt, kLong, kFloat, kDouble, kChar,
        kVarChar, kDate, kDatetime,
        kDatetimeInterval, kYearMonthInterval };

  std::vector<OperationSignatureLitePtr> signatures;
  signatures.reserve(tids.size());

  for (const TypeID tid : tids) {
    signatures.emplace_back(
        OperationSignatureLite::Create(
            getShortName(), {tid, kVarChar}, {false, true}));
  }
  return signatures;
}

bool CastOperation::canApplyToSignature(const OperationSignaturePtr &signature,
                                        std::string *diagnostic_message) const {
  DCHECK_EQ(2u, signature->getNumParameters());
  DCHECK(signature->isStaticParameter(1));
  DCHECK_EQ(kVarChar, signature->getParameterType(1).getTypeID());

  const std::string target_type_name(
      static_cast<const char*>(signature->getStaticParameterValue(1).getOutOfLineData()));
  const Type *target_type = TypeFactory::ParseTypeFromString(target_type_name);
  if (target_type == nullptr) {
    *diagnostic_message = "Unrecognized type name \"" + target_type_name + "\"";
    return false;
  }

  if (QUICKSTEP_EQUALS_ANY_CONSTANT(target_type->getTypeID(), kChar, kVarChar)) {
    return true;
  }

  const Type &source_type = signature->getParameterType(0);
  if (!target_type->isCoercibleFrom(source_type)) {
    *diagnostic_message = "Could not cast value of type \"" + source_type.getName() +
                          "\" to \"" + target_type_name + "\"";
    return false;
  }
  return true;
}

const Type& CastOperation::resultTypeForSignature(
    const OperationSignaturePtr &signature) const {
  DCHECK(UnaryOperation::canApplyToSignature(signature));
  return *ParseTypeFromVarCharValue(signature->getStaticParameterValue(1u));
}

UncheckedUnaryOperator* CastOperation::makeUncheckedUnaryOperatorForSignature(
    const OperationSignaturePtr &signature) const {
  DCHECK(UnaryOperation::canApplyToSignature(signature));
  const Type &source_type = signature->getParameterType(0);
  const Type &target_type =
      *ParseTypeFromVarCharValue(signature->getStaticParameterValue(1));

  if (source_type.getSuperTypeID() == Type::kNumeric &&
      target_type.getSuperTypeID() == Type::kNumeric) {
    return meta::MultipleDispatcher<TypeIDSequenceNumeric>
               ::repeat<2>
               ::set_next<meta::BoolDispatcher>
               ::InvokeOn(
        source_type.getTypeID(),
        target_type.getTypeID(),
        source_type.isNullable(),
        [&](auto typelist) -> UncheckedUnaryOperator* {
      constexpr TypeID source_type_id = decltype(typelist)::template at<0>::value;
      constexpr TypeID target_type_id = decltype(typelist)::template at<1>::value;

      constexpr bool source_nullable = decltype(typelist)::template at<2>::value;

      using CastFunctor = CastNumericToNumericFunctor<
          typename TypeTrait<source_type_id>::TypeClass,
          typename TypeTrait<target_type_id>::TypeClass>;

      return new UncheckedUnaryOperatorCodeGen<CastFunctor, source_nullable>(
          source_type, target_type);
    });
  }

  if (QUICKSTEP_EQUALS_ANY_CONSTANT(target_type.getTypeID(), kChar, kVarChar)) {
    return meta::MultipleDispatcher<TypeIDSequenceCxxInlinePod>
               ::set_next<meta::MultipleDispatcher<TypeIDSequenceAsciiString>>
               ::set_next<meta::BoolDispatcher>
               ::InvokeOn(
        source_type.getTypeID(),
        target_type.getTypeID(),
        source_type.isNullable(),
        [&](auto typelist) -> UncheckedUnaryOperator* {
      constexpr TypeID source_type_id = decltype(typelist)::template at<0>::value;
      constexpr TypeID target_type_id = decltype(typelist)::template at<1>::value;
      DCHECK(QUICKSTEP_EQUALS_ANY_CONSTANT(target_type_id, kChar, kVarChar));

      constexpr bool source_nullable = decltype(typelist)::template at<2>::value;

      using SourceType = typename TypeTrait<source_type_id>::TypeClass;
      using TargetType = typename TypeTrait<target_type_id>::TypeClass;
      using CastFunctor =
          typename std::conditional<target_type_id == kChar,
                                    CastAnyToCharFunctor<SourceType>,
                                    CastAnyToVarCharFunctor<SourceType>>::type;

      return new UncheckedUnaryOperatorCodeGen<CastFunctor, source_nullable>(
          source_type, target_type,
          static_cast<const SourceType&>(source_type),
          static_cast<const TargetType&>(target_type));
    });
  }

  LOG(FATAL) << "To be supported";
}

const Type* CastOperation::ParseTypeFromVarCharValue(const TypedValue &value) {
  DCHECK_EQ(kVarChar, value.getTypeID());
  const std::string type_name(static_cast<const char*>(value.getOutOfLineData()));
  return TypeFactory::ParseTypeFromString(type_name);
}

}  // namespace quickstep
