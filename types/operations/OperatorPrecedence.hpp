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

#ifndef QUICKSTEP_TYPES_OPERATIONS_OPERATOR_PRECEDENCE_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_OPERATOR_PRECEDENCE_HPP_

#include <cstddef>

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

// ----------------------------------------------------------------------------
// Operator precedence constant values.

static constexpr std::size_t kOperatorPrecedenceAtomicEntity = 0;
static constexpr std::size_t kOperatorPrecedenceFunctionCall = 0;

static constexpr std::size_t kOperatorPrecedenceCast = 100;

static constexpr std::size_t kOperatorPrecedenceUnaryMinus = 200;

static constexpr std::size_t kOperatorPrecedenceMultiplication = 400;
static constexpr std::size_t kOperatorPrecedenceDivision = 400;
static constexpr std::size_t kOperatorPrecedenceModulus = 400;

static constexpr std::size_t kOperatorPrecedenceAddition = 500;
static constexpr std::size_t kOperatorPrecedenceSubtraction = 500;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_OPERATOR_PRECEDENCE_HPP_
