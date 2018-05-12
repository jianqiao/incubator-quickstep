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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_OPERATION_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_OPERATION_HPP_

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "catalog/CatalogTypedefs.hpp"
#include "storage/StorageBlockInfo.hpp"
#include "types/operations/Operation.hpp"
#include "types/operations/Operation.pb.h"
#include "types/TypedValue.hpp"
#include "types/operations/OperationSignature.hpp"
#include "utility/Macros.hpp"

namespace quickstep {

class ColumnVector;
class Type;
class ValueAccessor;

/** \addtogroup Types
 *  @{
 */

class UnaryOperation;
typedef std::shared_ptr<const UnaryOperation> UnaryOperationPtr;

/**
 * @brief A unary operator which can be quickly applied to data items WITHOUT
 *        checking their type.
 **/
class UncheckedUnaryOperator {
 public:
  /**
   * @brief Virtual destructor.
   **/
  virtual ~UncheckedUnaryOperator() {
  }

  /**
   * @brief Apply to a TypedValue without type-checking.
   *
   * @param argument The TypedValue to apply to.
   * @return The literal result of the operation.
   **/
  virtual TypedValue applyToTypedValue(const TypedValue &argument) const = 0;

  /**
   * @brief Apply to a vector of values without type-checking.
   *
   * @param argument The argument ColumnVector to apply to.
   * @return A ColumnVector of literal results of the operation.
   **/
  virtual ColumnVector* applyToColumnVector(const ColumnVector &argument) const = 0;

  /**
   * @brief Apply to an attribute of all tuples in a ValueAccessor.
   *
   * @param accessor The ValueAccessor to apply to.
   * @param argument_attr_id The attribute ID of the argument in accessor.
   * @return A ColumnVector of literal results of the operation.
   **/
  virtual ColumnVector* applyToValueAccessor(ValueAccessor *accessor,
                                             const attribute_id argument_attr_id) const = 0;

 protected:
  UncheckedUnaryOperator() {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UncheckedUnaryOperator);
};

/**
 * @brief An operation which has a single argument and returns a single value.
 **/
class UnaryOperation : public Operation {
 public:
  /**
   * @brief Create an UncheckedUnaryOperator which can apply to items of the
   *        specified signature.
   *
   * @param signature The signature of argument to apply to.
   * @return An UncheckedUnaryOperator which applies this UnaryOperation to
   *         the specified signature.
   **/
  virtual UncheckedUnaryOperator* makeUncheckedUnaryOperatorForSignature(
      const OperationSignaturePtr &signature) const = 0;

  /**
   * @brief Generate expression string of this unary operation applied to the
   *        specified argument.
   *
   * @param signature The signature of argument to apply to.
   * @param argument The argument.
   * @param argument_precedence The argument's top level operator precedence.
   * @param The expression string of this unary operation applied to the
   *        specified argument.
   */
  virtual std::string formatExpression(const OperationSignaturePtr &signature,
                                       const std::string &argument,
                                       const std::size_t argument_precedence) const;

 protected:
  UnaryOperation() : Operation(Operation::kUnaryOperation) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(UnaryOperation);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_OPERATION_HPP_
