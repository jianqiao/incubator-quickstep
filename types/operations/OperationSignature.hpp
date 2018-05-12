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

#ifndef QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_HPP_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "types/Type.hpp"
#include "types/TypedValue.hpp"
#include "types/operations/Operation.pb.h"
#include "types/operations/OperationSignatureLite.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

class OperationSignature;
typedef std::shared_ptr<const OperationSignature> OperationSignaturePtr;

class OperationSignature {
 public:
  /**
   * @brief Serialize this operation signature as Protocol Buffer.
   *
   * @return The Protocol Buffer representation of this operation signature.
   **/
  serialization::OperationSignature getProto() const;

  /**
   * @brief Reconstruct an operation signature from its serialized Protocol
   *        Buffer form.
   *
   * @param proto The Protocol Buffer serialization of an operation signature,
   *        previously produced by getProto().
   * @return The operation signature described by proto.
   **/
  static OperationSignaturePtr ReconstructFromProto(
      const serialization::OperationSignature &proto);

  /**
   * @brief Check whether a serialization::OperationSignature is fully-formed
   *        and all parts are valid.
   *
   * @param proto The Protocol Buffer serialization of an operation signature,
   *        previously produced by getProto().
   * @return Whether proto is fully-formed and valid.
   **/
  static bool ProtoIsValid(const serialization::OperationSignature &proto);

  /**
   * @brief Get the lite version of this operation signature.
   *
   * @return The lite version of this operation signature.
   */
  const OperationSignatureLitePtr& getSignatureLite() const {
    return sig_lite_;
  }

  /**
   * @brief Get the operation name.
   *
   * @return The operation name.
   */
  const std::string& getOperationName() const {
    return sig_lite_->getOperationName();
  }

  /**
   * @brief Get the number of operation parameters.
   *
   * @return The number of operation parameters.
   */
  std::size_t getNumParameters() const {
    return param_types_.size();
  }

  /**
   * @brief Get the number of operation parameters that are static.
   *
   * @return The number of operation parameters that are static.
   */
  std::size_t getNumStaticParameters() const {
    return sig_lite_->getNumStaticParameters();
  }

  /**
   * @brief Get the parameter type at the specified position.
   *
   * @param pos The position.
   * @return The parameter type at the specified position.
   */
  const Type& getParameterType(const std::size_t pos) const {
    DCHECK_LT(pos, param_types_.size());
    return *param_types_[pos];
  }

  /**
   * @brief Check whether the parameter at the specified position is a static
   *        parameter.
   *
   * @param pos The position.
   * @return True if the parameter at the specified position is a static
   *         parameter, false otherwise.
   */
  bool isStaticParameter(const std::size_t pos) const {
    return sig_lite_->isStaticParameter(pos);
  }

  /**
   * @brief Get the static parameter value at the specified position.
   *
   * @param pos The position.
   * @return The static parameter value at the specified position.
   */
  const TypedValue& getStaticParameterValue(const std::size_t pos) const {
    DCHECK(isStaticParameter(pos));
    return static_param_values_[pos];
  }

  /**
   * @brief Check whether this operation signature equals the other.
   *
   * @param other The other operation signature.
   * @return True if this signature equals the other, false otherwise.
   */
  bool operator==(const OperationSignature &other) const;

  /**
   * @brief Check whether this operation signature does not equal the other.
   *
   * @param other The other operation signature.
   * @return True if this signature does not equal the other, false otherwise.
   */
  bool operator!=(const OperationSignature &r) const {
    return !(*this == r);
  }

  /**
   * @brief Coerce this operation signature in conformity with the target lite
   *        signature.
   *
   * @param The target lite operation signature.
   * @return The coerced operation signature.
   */
  OperationSignaturePtr getCoercedSignature(
      const OperationSignatureLitePtr &target_signature) const;

  /**
   * @brief Get the hash code of this operation signature.
   * @note Two operation signatures are guaranteed to have the same hash code
   *       if they equal (==) each other.
   *
   * @return The hash code of this operation signature.
   */
  std::size_t getHash() const;

  /**
   * @brief Get a human readable representation of this operation signature.
   *
   * @return A human readable representation of this operation signature.
   */
  std::string toString() const;

  /**
   * @brief Create an operation signature.
   * @note This factory method assumes that all parameters are non-static.
   *
   * @param sig_lite The lite version of the operation signature.
   * @param param_types The parameter types.
   * @return An operation signature.
   */
  static OperationSignaturePtr Create(const OperationSignatureLitePtr &sig_lite,
                                      const std::vector<const Type*> &param_types);

  /**
   * @brief Create an operation signature.
   *
   * @param sig_lite The lite version of the operation signature.
   * @param param_types The parameter types.
   * @param static_param_values The static parameter values. This vector should
   *        have the same length as \p param_types. For non-static parameters,
   *        the values at corresponding positions can be anything.
   * @return An operation signature.
   */
  static OperationSignaturePtr Create(const OperationSignatureLitePtr &sig_lite,
                                      const std::vector<const Type*> &param_types,
                                      const std::vector<TypedValue> &static_param_values);

  /**
   * @brief Create an operation signature.
   * @note This factory method assumes that all parameters are non-static.
   *
   * @param op_name The operation name.
   * @param variadic_param_types The parameter types.
   * @return An operation signature.
   */
  template <typename ...TypeClass>
  static OperationSignaturePtr Create(const std::string &op_name,
                                      const TypeClass &...variadic_param_types);

 private:
  OperationSignature(const OperationSignatureLitePtr &sig_lite,
                     const std::vector<const Type*> &param_types,
                     const std::vector<TypedValue> &static_param_values)
      : sig_lite_(DCHECK_NOTNULL(sig_lite)),
        param_types_(param_types),
        static_param_values_(static_param_values) {
    DCHECK_EQ(param_types_.size(), sig_lite_->getNumParameters());
    DCHECK_EQ(static_param_values_.size(), sig_lite_->getNumParameters());
  }

  const OperationSignatureLitePtr sig_lite_;
  const std::vector<const Type*> param_types_;
  const std::vector<TypedValue> static_param_values_;

  DISALLOW_COPY_AND_ASSIGN(OperationSignature);
};

/** @} */

// ----------------------------------------------------------------------------
// Implementations of templated methods follow:

template <typename ...TypeClass>
OperationSignaturePtr OperationSignature::Create(
    const std::string &op_name,
    const TypeClass &...variadic_param_types) {
  std::vector<const Type*> param_types{&variadic_param_types...};
  std::vector<TypeID> param_type_ids;
  param_type_ids.reserve(param_types.size());
  for (const Type *pt : param_types) {
    param_type_ids.emplace_back(pt->getTypeID());
  }
  return OperationSignature::Create(
      OperationSignatureLite::Create(op_name, param_type_ids), param_types);
}

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_HPP_
