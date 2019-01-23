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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_ASCII_STRING_UNARY_FUNCTORS_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_ASCII_STRING_UNARY_FUNCTORS_HPP_

#include <cstdlib>
#include <cstring>
#include <string>

#include "types/CharType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/VarCharType.hpp"
#include "types/Type.hpp"
#include "types/TypeFactory.hpp"
#include "types/TypeID.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperationTraits.hpp"
#include "types/operations/OperatorPrecedence.hpp"
#include "types/operations/unary_operations/UnaryFunctor.hpp"
#include "types/port/strnlen.hpp"
#include "utility/meta/StringConstant.hpp"
#include "utility/meta/TypeList.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

namespace ascii_string_unary_operations {

template <typename TypeClass>
using ValuePointer = typename OperationResultTraits<TypeClass>::ValuePointer;

// Template for wrapping a function with CharType argument as a UnaryFunctor.
template <typename ResultType,
          void f(const char*, const std::size_t, ValuePointer<ResultType>),
          typename FunctionName>
struct CharStringUnaryFunctorTrait {
  class type : public UnaryFunctor<FunctionName,
                                   FunctionName,
                                   kOperatorPrecedenceFunctionCall,
                                   CharType,
                                   ResultType> {
   public:
    explicit type(const OperationSignaturePtr &signature)
        : max_argument_length_(GetArgumentType(*signature).getStringLength()) {}

    inline void operator()(const void *argument,
                           ValuePointer<ResultType> result) const {
      f(static_cast<const char*>(argument), max_argument_length_, result);
    }

//    static const CharType& ResultTypeForSignature(
//        const OperationSignaturePtr &signature) {
//      return GetArgumentType(*signature);
//    }

   private:
    static const CharType& GetArgumentType(const OperationSignature &signature) {
      DCHECK_GE(signature.getNumParameters(), 1u);
      const Type &argument_type = signature.getParameterType(0);
      DCHECK_EQ(kChar, argument_type.getTypeID());
      return static_cast<const CharType&>(argument_type);
    }
    const std::size_t max_argument_length_;
  };
};

template <typename ResultType,
          void f(const char *, const std::size_t, ValuePointer<ResultType>),
          typename FunctionName>
using CharStringUnaryFunctor =
    typename CharStringUnaryFunctorTrait<ResultType, f, FunctionName>::type;

// Template for wrapping a function with VarCharType argument as a UnaryFunctor.
template <typename ResultType,
          void f(const char*, ValuePointer<ResultType>),
          typename FunctionName>
struct VarCharStringUnaryFunctorTrait {
  struct type : public UnaryFunctor<FunctionName,
                                    FunctionName,
                                    kOperatorPrecedenceFunctionCall,
                                    VarCharType,
                                    ResultType> {
    inline void operator()(const TypedValue &argument,
                           ValuePointer<ResultType> result) const {
      f(static_cast<const char*>(argument.getOutOfLineData()), result);
    }

//    static const VarCharType& ResultTypeForSignature(
//        const OperationSignaturePtr &signature) {
//      DCHECK_GE(signature->getNumParameters(), 1u);
//      const Type &argument_type = signature->getParameterType(0);
//      DCHECK_EQ(kVarChar, argument_type.getTypeID());
//      return static_cast<const VarCharType&>(argument_type);
//    }
  };
};

template <typename ResultType,
          void f(const char*, ValuePointer<ResultType>),
          typename FunctionName>
using VarCharStringUnaryFunctor =
    typename VarCharStringUnaryFunctorTrait<ResultType, f, FunctionName>::type;

// ----------------------------------------------------------------------------
// List of "length" functions.

template <typename ResultType>
inline void GetCharLength(const char *data,
                          const std::size_t maxlen,
                          typename ResultType::cpptype *result) {
  *result = strnlen(data, maxlen);
}

template <typename ResultType>
inline void GetVarCharLength(const char *data,
                             typename ResultType::cpptype *result) {
  *result = std::strlen(data);
}

using LengthFunctors = meta::TypeList<
    CharStringUnaryFunctor<IntType, GetCharLength<IntType>, STR_CONST8("length")>,
    VarCharStringUnaryFunctor<IntType, GetVarCharLength<IntType>, STR_CONST8("length")>>;

// ----------------------------------------------------------------------------
// List of "upper" / "lower" functions.

template <int transform(int)>
inline void TransformChar(const char *data,
                          const std::size_t maxlen,
                          void *result) {
  const std::size_t len = strnlen(data, maxlen);
  char *mm = static_cast<char*>(result);
  for (std::size_t i = 0; i < len; ++i) {
    mm[i] = transform(data[i]);
  }
  if (len < maxlen) {
    mm[len] = 0;
  }
}

template <int transform(int)>
inline void TransformVarChar(const char *data,
                             TypedValue *result) {
  const std::size_t len = strlen(data);
  char *mm = static_cast<char*>(std::malloc(len+1));
  for (std::size_t i = 0; i < len; ++i) {
    mm[i] = transform(data[i]);
  }
  mm[len] = 0;
  *result = TypedValue::CreateWithOwnedData(kVarChar, mm, len+1);
}

using TransformFunctors = meta::TypeList<
    CharStringUnaryFunctor<CharType, TransformChar<std::tolower>, STR_CONST8("lower")>,
    CharStringUnaryFunctor<CharType, TransformChar<std::toupper>, STR_CONST8("upper")>,
    VarCharStringUnaryFunctor<VarCharType, TransformVarChar<std::tolower>, STR_CONST8("lower")>,
    VarCharStringUnaryFunctor<VarCharType, TransformVarChar<std::toupper>, STR_CONST8("upper")>>;

}  // namespace ascii_string_unary_operations

// ----------------------------------------------------------------------------
// All-in-one packing.

using AsciiStringUnaryFunctors =
    meta::TypeList<ascii_string_unary_operations::LengthFunctors,
                   ascii_string_unary_operations::TransformFunctors>;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_ASCII_STRING_UNARY_FUNCTORS_HPP_
