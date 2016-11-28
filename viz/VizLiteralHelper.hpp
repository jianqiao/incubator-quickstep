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

#ifndef QUICKSTEP_VIZ_LITERAL_HELPER_HPP_
#define QUICKSTEP_VIZ_LITERAL_HELPER_HPP_

#include <exception>

#include "utility/Macros.hpp"
#include "types/TypedValue.hpp"

namespace quickstep {

namespace viz {

/** \addtogroup Viz
 *  @{
 */

class NonNumerical : public std::exception {
  public:
    virtual const char* what() const throw() {
      return "NonNumerical: Does not support non numerical type";
    }
};

class VizLiteralHelper {
 public:
  static double getLiteral(const TypedValue &val) {
    switch(val.getTypeID()) {
      case kInt:
        return static_cast<double>(val.getLiteral<int>());
      case kLong:
        return static_cast<double>(val.getLiteral<int64_t>());
      case kFloat:
        return static_cast<double>(val.getLiteral<float>());
      case kDouble:
        return static_cast<double>(val.getLiteral<double>());
      default:
        // does not support non numerical types
        throw NonNumerical();
    }
  }

 private:
  VizLiteralHelper() {}

  DISALLOW_COPY_AND_ASSIGN(VizLiteralHelper);
};

/** @} */

}  // namespace viz
}  // namespace quickstep

#endif  // QUICKSTEP_VIZ_LITERAL_HELPER_HPP_
