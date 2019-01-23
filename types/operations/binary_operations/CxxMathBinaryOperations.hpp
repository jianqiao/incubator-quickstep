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

#ifndef QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_CXX_MATH_BINARY_OPERATIONS_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_CXX_MATH_BINARY_OPERATIONS_HPP_

#include <cmath>
#include <string>

#include "types/DoubleType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/operations/OperatorPrecedence.hpp"
#include "types/operations/binary_operations/BinaryFunctor.hpp"
#include "utility/meta/StringConstant.hpp"
#include "utility/meta/TypeList.hpp"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

template <typename LeftType, typename RightType, typename ResultType,
          typename ResultType::cpptype f(typename LeftType::cpptype,
                                         typename RightType::cpptype),
          typename FunctionName>
struct CxxMathBinaryFunctorTrait {
  struct type : public BinaryFunctor<FunctionName,
                                     FunctionName,
                                     kOperatorPrecedenceFunctionCall,
                                     LeftType,
                                     RightType,
                                     ResultType> {
    inline void operator()(const typename LeftType::cpptype *left,
                           const typename RightType::cpptype *right,
                           typename ResultType::cpptype *result) const {
      *result = f(*left, *right);
    }
  };
};

template <typename LeftType, typename RightType, typename ResultType,
          typename ResultType::cpptype f(typename LeftType::cpptype,
                                         typename RightType::cpptype),
          typename FunctionName>
using CxxMathBinaryFunctor =
    typename CxxMathBinaryFunctorTrait<
        LeftType, RightType, ResultType, f, FunctionName>::type;

// ----------------------------------------------------------------------------
// List of C standard library math functions.

using PowFunctors = meta::TypeList<
    CxxMathBinaryFunctor<FloatType, FloatType, FloatType, std::pow, STR_CONST8("pow")>,
    CxxMathBinaryFunctor<DoubleType, DoubleType, DoubleType, std::pow, STR_CONST8("pow")>>;

// ----------------------------------------------------------------------------
// All-in-one packing.

using CxxMathBinaryFunctors = meta::TypeList<PowFunctors>;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_CXX_MATH_BINARY_OPERATIONS_HPP_
