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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_CXX_MATH_UNARY_FUNCTORS_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_CXX_MATH_UNARY_FUNCTORS_HPP_

#include <cmath>
#include <string>

#include "types/DoubleType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/operations/OperatorPrecedence.hpp"
#include "types/operations/unary_operations/UnaryFunctor.hpp"
#include "utility/meta/StringConstant.hpp"
#include "utility/meta/TypeList.hpp"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

template <typename ArgumentType, typename ResultType,
          typename ResultType::cpptype f(typename ArgumentType::cpptype),
          typename FunctionName>
struct CxxMathUnaryFunctorTrait {
  struct type : public UnaryFunctor<FunctionName,
                                    FunctionName,
                                    kOperatorPrecedenceFunctionCall,
                                    ArgumentType,
                                    ResultType> {
    inline void operator()(const typename ArgumentType::cpptype *argument,
                           typename ResultType::cpptype *result) const {
      *result = f(*argument);
    }
  };
};

template <typename ValueType,
          typename ValueType::cpptype f(typename ValueType::cpptype),
          typename FunctionName>
using CxxMathUnaryFunctor =
    typename CxxMathUnaryFunctorTrait<ValueType, ValueType, f, FunctionName>::type;


// ----------------------------------------------------------------------------
// List of C standard library math functions.

using AbsFunctors =
    meta::TypeList<CxxMathUnaryFunctor<IntType, std::abs, STR_CONST8("abs")>,
                   CxxMathUnaryFunctor<LongType, std::abs, STR_CONST8("abs")>,
                   CxxMathUnaryFunctor<FloatType, std::abs, STR_CONST8("abs")>,
                   CxxMathUnaryFunctor<DoubleType, std::abs, STR_CONST8("abs")>>;

using SqrtFunctors =
    meta::TypeList<CxxMathUnaryFunctor<FloatType, std::sqrt, STR_CONST8("sqrt")>,
                   CxxMathUnaryFunctor<DoubleType, std::sqrt, STR_CONST8("sqrt")>>;

using ExpFunctors =
    meta::TypeList<CxxMathUnaryFunctor<FloatType, std::exp, STR_CONST8("exp")>,
                   CxxMathUnaryFunctor<DoubleType, std::exp, STR_CONST8("exp")>>;

using LogFunctors =
    meta::TypeList<CxxMathUnaryFunctor<FloatType, std::log, STR_CONST8("log")>,
                   CxxMathUnaryFunctor<DoubleType, std::log, STR_CONST8("log")>>;

using CeilFunctors =
    meta::TypeList<CxxMathUnaryFunctor<FloatType, std::ceil, STR_CONST8("ceil")>,
                   CxxMathUnaryFunctor<DoubleType, std::ceil, STR_CONST8("ceil")>>;

using FloorFunctors =
    meta::TypeList<CxxMathUnaryFunctor<FloatType, std::floor, STR_CONST8("floor")>,
                   CxxMathUnaryFunctor<DoubleType, std::floor, STR_CONST8("floor")>>;

using RoundFunctors =
    meta::TypeList<CxxMathUnaryFunctor<FloatType, std::round, STR_CONST8("round")>,
                   CxxMathUnaryFunctor<DoubleType, std::round, STR_CONST8("round")>>;

// ----------------------------------------------------------------------------
// All-in-one packing.

using CxxMathUnaryFunctors = meta::TypeList<AbsFunctors,
                                            SqrtFunctors,
                                            ExpFunctors,
                                            LogFunctors,
                                            CeilFunctors,
                                            FloorFunctors,
                                            RoundFunctors>;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_CXX_MATH_UNARY_FUNCTORS_HPP_
