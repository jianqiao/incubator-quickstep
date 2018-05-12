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

#ifndef QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_COERCION_PRECEDENCE_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_COERCION_PRECEDENCE_HPP_

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

class OperationSignatureCoercionPrecedence {
 public:
  enum CoercibilityLevel : std::uint8_t {
    kInvalid = 0,
    kCoercible,
    kSafelyCoercible,
    kExactMatch
  };

  OperationSignatureCoercionPrecedence()
      : regular_args_level_(kInvalid),
        static_args_level_(kInvalid),
        num_static_args_(0) {}

  OperationSignatureCoercionPrecedence(const CoercibilityLevel regular_args_level,
                                       const CoercibilityLevel static_args_level,
                                       const std::size_t num_static_args)
      : regular_args_level_(regular_args_level),
        static_args_level_(static_args_level),
        num_static_args_(num_static_args) {
    DCHECK_NE(regular_args_level_, kInvalid);
    DCHECK_NE(static_args_level_, kInvalid);
  }

  bool isValid() const {
    return regular_args_level_ != kInvalid && static_args_level_ != kInvalid;
  }

  bool operator==(const OperationSignatureCoercionPrecedence &other) const {
    return num_static_args_ == other.num_static_args_ &&
           regular_args_level_ == other.regular_args_level_ &&
           static_args_level_ == other.static_args_level_;
  }

  bool operator!=(const OperationSignatureCoercionPrecedence &other) const {
    return !(*this == other);
  }

  bool operator<(const OperationSignatureCoercionPrecedence &other) const {
    if (num_static_args_ != other.num_static_args_) {
      return num_static_args_ < other.num_static_args_;
    }
    if (regular_args_level_ != other.regular_args_level_) {
      return regular_args_level_ < other.regular_args_level_;
    }
    return static_args_level_ < other.static_args_level_;
  }

  static CoercibilityLevel GetUnifyingCoercibilityLevel(
      const CoercibilityLevel &first, const CoercibilityLevel &second) {
    return std::min(first, second);
  }

 private:
  CoercibilityLevel regular_args_level_;
  CoercibilityLevel static_args_level_;
  std::uint16_t num_static_args_;
};

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_COERCION_PRECEDENCE_HPP_
