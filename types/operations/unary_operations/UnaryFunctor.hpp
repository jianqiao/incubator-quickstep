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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_FUNCTOR_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_FUNCTOR_HPP_

#include <cstddef>
#include <memory>
#include <string>

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

// Forward declaration of the code gen class.
template <typename Functor>
class UnaryOperationCodeGen;

template <typename Name, typename ShortName,
          typename ParameterTypeIn, typename ResultTypeIn,
          bool argument_nullable = false, bool result_nullable = false>
struct UnaryFunctor {
  using ParameterType = ParameterTypeIn;
  using ResultType = ResultTypeIn;

  static constexpr Operation::OperationSuperTypeID
      kOperationSuperTypeID = Operation::kUnaryOperation;

  static constexpr bool kConsumesNullArgument = argument_nullable;
  static constexpr bool kProducesNullResult = result_nullable;

  template <typename Functor>
  using OperationCodeGen = UnaryOperationCodeGen<Functor>;

  static std::string GetName() {
    return Name::ToString();
  }

  static std::string GetShortName() {
    return ShortName::ToString();
  }
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_FUNCTOR_HPP_
