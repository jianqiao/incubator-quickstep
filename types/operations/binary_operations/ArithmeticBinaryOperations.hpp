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

#ifndef QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_ARITHMETIC_BINARY_OPERATIONS_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_ARITHMETIC_BINARY_OPERATIONS_HPP_

#include <functional>
#include <type_traits>

#include "types/DateType.hpp"
#include "types/DatetimeType.hpp"
#include "types/DatetimeIntervalType.hpp"
#include "types/DoubleType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/NumericTypeUnifier.hpp"
#include "types/YearMonthIntervalType.hpp"
#include "types/operations/OperatorPrecedence.hpp"
#include "types/operations/binary_operations/ArithmeticBinaryFunctorOverloads.hpp"
#include "types/operations/binary_operations/BinaryFunctor.hpp"
#include "utility/meta/StringConstant.hpp"
#include "utility/meta/TypeList.hpp"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

namespace arithmetic_binary_operations {

template <typename Name, typename ShortName, std::size_t operator_precedence,
          typename LeftType, typename RightType, typename ResultType,
          template <typename LeftCppType,
                    typename RightCppType,
                    typename Enable = void> class FunctorOverloads>
struct ArithmeticBinaryFunctor : public BinaryFunctor<Name,
                                                      ShortName,
                                                      operator_precedence,
                                                      LeftType,
                                                      RightType,
                                                      ResultType> {
  using LeftCppType = typename LeftType::cpptype;
  using RightCppType = typename RightType::cpptype;
  using ResultCppType = typename ResultType::cpptype;

  ArithmeticBinaryFunctor() {}

  inline void operator()(const LeftCppType *left,
                         const RightCppType *right,
                         ResultCppType *result) const {
    *result = functor(*left, *right);
  }
  const FunctorOverloads<LeftCppType, RightCppType> functor;
};

// ----------------------------------------------------------------------------
// Helper type traits.

// For a pair of numeric type parameters, make it a triplet by appending the
// unified numeric type of the pair.
template <typename Parameters>
struct PushBackInferredUnifiedNumericType {
  using Result = typename NumericTypeUnifier<
      typename Parameters::template at<0>,
      typename Parameters::template at<1>>::type;
  using type = typename Parameters::template push_back<Result>;
};

// For a pair of type parameters, make it a triplet by appending again the first
// element.
template <typename Parameters>
struct PushBackFirstElement {
  using type = typename Parameters::template push_back<typename Parameters::head>;
};

// For a list of signature triples, append a copy of each triplet with type
// parameters reversed.
template <typename SignatureTriplets>
struct SymmetrifyTrait {
  template <typename SignatureTriplet>
  struct ReverseParameters {
    using type = meta::TypeList<typename SignatureTriplet::template at<1>,
                                typename SignatureTriplet::template at<0>,
                                typename SignatureTriplet::template at<2>>;
  };
  using Mirrored = typename SignatureTriplets::template apply<ReverseParameters>;

  using type = typename SignatureTriplets::template append<Mirrored>
                                         ::template unique<>;
};

template <typename SignatureTriplets>
using Symmetrify = typename SymmetrifyTrait<SignatureTriplets>::type;

// For a low-level binary functor and a list of signature triples, wrap them
// into a list of corresponding Quickstep BinaryFunctor's.
template <typename Name, typename ShortName, std::size_t operator_precedence,
          typename SignatureTriplets,
          template <typename LeftCppType,
                    typename RightCppType,
                    typename Enable = void> class FunctorOverloads>
struct CreateBinaryFunctorsTrait {
  template <typename SignatureTriplet>
  struct Transform {
    using type = ArithmeticBinaryFunctor<Name, ShortName, operator_precedence,
                                         typename SignatureTriplet::template at<0>,
                                         typename SignatureTriplet::template at<1>,
                                         typename SignatureTriplet::template at<2>,
                                         FunctorOverloads>;
  };
  using type = typename SignatureTriplets::template apply<Transform>;
};

template <typename Name, typename ShortName, std::size_t operator_precedence,
          typename SignatureTriplets,
          template <typename LeftCppType,
                    typename RightCppType,
                    typename Enable = void> class FunctorOverloads>
using CreateBinaryFunctors =
    typename CreateBinaryFunctorsTrait<Name, ShortName, operator_precedence,
                                       SignatureTriplets,
                                       FunctorOverloads>::type;

// List of numeric types.
using NumericTypes = meta::TypeList<IntType, LongType, FloatType, DoubleType>;

// List of time interval types.
using TimeIntervalTypes = meta::TypeList<DatetimeIntervalType, YearMonthIntervalType>;

// All possible combinations of pairs of numeric types together with each pair's
// unified type as triplets. E.g.
// --
// [
//   [IntType, IntType, IntType],
//   [IntType, LongType, LongType],
//   ...
//   [FloatType, IntType, FloatType],
//   [FloatType, LongType, DoubleType],
//   ...
// ]
using BinaryNumericUnifiedSignatures = NumericTypes::cartesian_product<NumericTypes>
                                                   ::apply<PushBackInferredUnifiedNumericType>;

// ----------------------------------------------------------------------------
// List of add functions.

// Signature triplets for add operation on date types.
using BinaryDateAddSignatures = Symmetrify<
    meta::TypeList<meta::TypeList<DateType, YearMonthIntervalType>,
                   meta::TypeList<DatetimeType, DatetimeIntervalType>,
                   meta::TypeList<DatetimeType, YearMonthIntervalType>,
                   meta::TypeList<DatetimeIntervalType, DatetimeIntervalType>,
                   meta::TypeList<YearMonthIntervalType, YearMonthIntervalType>>
        ::apply<PushBackFirstElement>>;

// All signature triplets for add operation.
using BinaryAddSignatures =
    meta::TypeList<BinaryNumericUnifiedSignatures,
                   BinaryDateAddSignatures>
        ::flatten_once<>;

// Add functors.
using AddFunctors = CreateBinaryFunctors<STR_CONST8("Add"),
                                         STR_CONST8("+"),
                                         kOperatorPrecedenceAddition,
                                         BinaryAddSignatures,
                                         AddFunctorOverloads>;

// ----------------------------------------------------------------------------
// List of subtract functions.

// Signature triplets for subtract operation on date types.
using BinaryDateSubtractSignatures = meta::TypeList<
    meta::TypeList<DateType, YearMonthIntervalType, DateType>,
    meta::TypeList<DatetimeType, DatetimeIntervalType, DatetimeType>,
    meta::TypeList<DatetimeType, YearMonthIntervalType, DatetimeType>,
    meta::TypeList<DatetimeType, DatetimeType, DatetimeIntervalType>,
    meta::TypeList<DatetimeIntervalType, DatetimeIntervalType, DatetimeIntervalType>,
    meta::TypeList<YearMonthIntervalType, YearMonthIntervalType, YearMonthIntervalType>>;

// All signature triplets for subtract operation.
using BinarySubtractSignatures =
    meta::TypeList<BinaryNumericUnifiedSignatures,
                   BinaryDateSubtractSignatures>
        ::flatten_once<>;

// Subtract functors.
using SubtractFunctors = CreateBinaryFunctors<STR_CONST8("Subtract"),
                                              STR_CONST8("-"),
                                              kOperatorPrecedenceSubtraction,
                                              BinarySubtractSignatures,
                                              SubtractFunctorOverloads>;

// ----------------------------------------------------------------------------
// List of multiply functions.

// Signature triplets for multiply operation on date types.
using BinaryDateMultiplySignatures = Symmetrify<
    TimeIntervalTypes::cartesian_product<NumericTypes>
                     ::apply<PushBackFirstElement>>;

// All signature triplets for multiply operation.
using BinaryMultiplySignatures =
    meta::TypeList<BinaryNumericUnifiedSignatures,
                   BinaryDateMultiplySignatures>
        ::flatten_once<>;

// Multiply functors.
using MultiplyFunctors = CreateBinaryFunctors<STR_CONST8("Multiply"),
                                              STR_CONST8("*"),
                                              kOperatorPrecedenceMultiplication,
                                              BinaryMultiplySignatures,
                                              MultiplyFunctorOverloads>;

// ----------------------------------------------------------------------------
// List of divide functions.

// Signature triplets for divide operation on date types.
using BinaryDateDivideSignatures =
    TimeIntervalTypes::cartesian_product<NumericTypes>
                     ::apply<PushBackFirstElement>;

// All signature triplets for divide operation.
using BinaryDivideSignatures =
    meta::TypeList<BinaryNumericUnifiedSignatures,
                   BinaryDateDivideSignatures>
        ::flatten_once<>;

// Divide functors.
using DivideFunctors = CreateBinaryFunctors<STR_CONST8("Divide"),
                                            STR_CONST8("/"),
                                            kOperatorPrecedenceDivision,
                                            BinaryDivideSignatures,
                                            DivideFunctorOverloads>;

// ----------------------------------------------------------------------------
// List of modulo functions.

// All signature triplets for modulo operation.
using BinaryModuloSignatures = BinaryNumericUnifiedSignatures;

// Modulo functors.
using ModuloFunctors = CreateBinaryFunctors<STR_CONST8("Modulo"),
                                            STR_CONST8("%"),
                                            kOperatorPrecedenceModulus,
                                            BinaryModuloSignatures,
                                            ModuloFunctorOverloads>;

}  // namespace arithmetic_binary_operations_helper

// ----------------------------------------------------------------------------
// All-in-one packing.

using ArithmeticBinaryFunctors =
    meta::TypeList<arithmetic_binary_operations::AddFunctors,
                   arithmetic_binary_operations::SubtractFunctors,
                   arithmetic_binary_operations::MultiplyFunctors,
                   arithmetic_binary_operations::DivideFunctors,
                   arithmetic_binary_operations::ModuloFunctors>;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_ARITHMETIC_BINARY_OPERATIONS_HPP_
