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

#ifndef QUICKSTEP_TYPES_OPERATIONS_OPERATION_FACTORY_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_OPERATION_FACTORY_HPP_

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "types/TypeID.hpp"
#include "types/operations/Operation.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperationSignatureLite.hpp"
#include "types/operations/binary_operations/BinaryOperation.hpp"
#include "types/operations/unary_operations/UnaryOperation.hpp"
#include "utility/HashPair.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

class Type;

/** \addtogroup Types
 *  @{
 */

class OperationFactory {
 public:
  /**
   * @brief Get the operation for a lite operation signature.
   *
   * @param signature The lite operation signature.
   * @return The operation for the specified lite signature.
   */
  static const Operation& GetOperation(
      const OperationSignatureLitePtr &signature);

  /**
   * @brief Get the unary operation for a lite operation signature.
   *
   * @param signature The lite operation signature.
   * @return The unary operation for the specified lite signature.
   */
  static const UnaryOperation& GetUnaryOperation(
      const OperationSignatureLitePtr &signature);

  /**
   * @brief Get the binary operation for a lite operation signature.
   *
   * @param signature The lite operation signature.
   * @return The binary operation for the specified lite signature.
   */
  static const BinaryOperation& GetBinaryOperation(
      const OperationSignatureLitePtr &signature);

  /**
   * @brief Perform overload resolution for a query-time operation signature.
   *
   * @param signature The signature to resolve.
   * @param diagnostic_message Diagnostic message to output for situations
   *        where the signature can not be resolved.
   * @return The original signature if there is an exact match, or a coerced
   *         signature if implicit type conversion took place, or nullptr if
   *         the signature can not be resolved.
   */
  static OperationSignaturePtr ResolveSignature(
      const OperationSignaturePtr &signature, std::string *diagnostic_message);

  /**
   * @brief Check if there exists an operation for a lite operation signature.
   *
   * @param signature The lite operation signature.
   * @return True if there exists an operation for the specified lite signature,
   *         false otherwise.
   */
  static bool HasOperation(const OperationSignatureLitePtr &signature);

  /**
   * @brief Check if there exists operation(s) of the specified name and arity.
   *
   * @param op_name The name to check.
   * @param arity The number of arguments to the operation.
   * @return True if there exists operation(s) of the specified name and arity,
   *         false otherwise.
   */
  static bool HasOperation(const std::string &op_name, const std::size_t arity);

  // Singleton instance.
  static const OperationFactory& Instance();

 private:
  enum class ResolveStatus {
    kSuccess = 0,
    kError,
    kNotFound
  };

  OperationFactory();

  // Register an operation into operation factory.
  void registerOperation(const OperationPtr &operation);

  // Create an instance from the template operation class and register it into
  // operation factory.
  template <typename OperationClass>
  void registerOperation();

  // Use the code gen (template expansion) approach to derive operation classes
  // from a functor class or a list of functor classes. Then create operation
  // instances and register them into operation factory.
  template <typename ...FunctorClasses>
  void registerFunctorsCodeGen();

  OperationSignaturePtr resolveInternal(const OperationSignaturePtr &signature,
                                        std::string *diagnostic_message) const;

  ResolveStatus resolveExactMatch(const OperationSignaturePtr &signature,
                                  OperationSignaturePtr *resolved,
                                  std::string *diagnostic_message) const;

  ResolveStatus resolvePartialMatch(const OperationSignaturePtr &signature,
                                    OperationSignaturePtr *resolved,
                                    std::string *diagnostic_message) const;

  std::unordered_map<OperationSignatureLitePtr,
                     OperationPtr,
                     OperationSignatureLitePtrHash,
                     OperationSignatureLitePtrEqual> operations_;

  std::unordered_map<std::pair<std::string, std::size_t>,
                     std::vector<OperationSignatureLitePtr>> name_arity_index_;

  DISALLOW_COPY_AND_ASSIGN(OperationFactory);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_OPERATION_FACTORY_HPP_
