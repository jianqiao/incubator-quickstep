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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_ARITHMETIC_UNARY_OPERATIONS_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_ARITHMETIC_UNARY_OPERATIONS_HPP_

#include "types/DatetimeIntervalType.hpp"
#include "types/DoubleType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/YearMonthIntervalType.hpp"
#include "types/operations/unary_operations/UnaryFunctor.hpp"
#include "utility/meta/StringConstant.hpp"
#include "utility/meta/TypeList.hpp"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

template <typename TypeClass>
struct NegateFunctor : public UnaryFunctor<STR_CONST8("Negate"),
                                           STR_CONST8("-"),
                                           TypeClass,
                                           TypeClass> {
  using ValueType = typename TypeClass::cpptype;
  inline void operator()(const ValueType *argument, ValueType *result) const {
    *result = -(*argument);
  }
};

template <typename TypeClass>
struct SgnFunctor : public UnaryFunctor<STR_CONST8("Sgn"),
                                        STR_CONST8("Sgn"),
                                        TypeClass,
                                        IntType> {
  using ValueType = typename TypeClass::cpptype;
  inline void operator()(const ValueType *argument, int *result) const {
    *result = (*argument > 0) - (*argument < 0);
  }
};

using NegateFunctors =
    meta::TypeList<IntType, LongType, FloatType, DoubleType,
                   DatetimeIntervalType, YearMonthIntervalType>
        ::apply<meta::TraitWrapper<NegateFunctor>::type>;

using SgnFunctors =
    meta::TypeList<IntType, LongType, FloatType, DoubleType>
        ::apply<meta::TraitWrapper<SgnFunctor>::type>;

using ArithmeticUnaryFunctors =
    meta::TypeList<NegateFunctors, SgnFunctors>;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_ARITHMETIC_UNARY_OPERATIONS_HPP_
