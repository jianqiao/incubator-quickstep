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

#ifndef QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_LITE_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_LITE_HPP_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "types/TypeID.hpp"
#include "types/operations/Operation.pb.h"
#include "types/operations/OperationSignatureCoercionPrecedence.hpp"
#include "utility/StringUtil.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

class OperationSignatureLite;
typedef std::shared_ptr<const OperationSignatureLite> OperationSignatureLitePtr;

class OperationSignatureLite {
 public:
  /**
   * @brief Serialize this lite operation signature as Protocol Buffer.
   *
   * @return The Protocol Buffer representation of this lite operation signature.
   **/
  serialization::OperationSignatureLite getProto() const;

  /**
   * @brief Reconstruct a lite operation signature from its serialized Protocol
   *        Buffer form.
   *
   * @param proto The Protocol Buffer serialization of a lite operation signature,
   *        previously produced by getProto().
   * @return The lite operation signature described by proto.
   **/
  static OperationSignatureLitePtr ReconstructFromProto(
      const serialization::OperationSignatureLite &proto);

  /**
   * @brief Check whether a serialization::OperationSignatureLite is fully-formed
   *        and all parts are valid.
   *
   * @param proto The Protocol Buffer serialization of a lite operation signature,
   *        previously produced by getProto().
   * @return Whether proto is fully-formed and valid.
   **/
  static bool ProtoIsValid(const serialization::OperationSignatureLite &proto);

  /**
   * @brief Get the operation name.
   *
   * @return The operation name.
   */
  const std::string& getOperationName() const {
    return op_name_;
  }

  /**
   * @brief Get the number of operation parameters.
   *
   * @return The number of operation parameters.
   */
  std::size_t getNumParameters() const {
    return param_type_ids_.size();
  }

  /**
   * @brief Get the number of operation parameters that are static.
   *
   * @return The number of operation parameters that are static.
   */
  std::size_t getNumStaticParameters() const {
    return num_static_params_;
  }

  /**
   * @brief Get the parameter type id at the specified position.
   *
   * @param pos The position.
   * @return The parameter type id at the specified position.
   */
  TypeID getParameterTypeID(const std::size_t pos) const {
    DCHECK_LT(pos, param_type_ids_.size());
    return param_type_ids_[pos];
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
    DCHECK_LT(pos, param_is_static_.size());
    return param_is_static_[pos];
  }

  /**
   * @brief Check whether this lite operation signature equals the other.
   *
   * @param other The other lite operation signature.
   * @return True if this signature equals the other, false otherwise.
   */
  bool operator==(const OperationSignatureLite &other) const {
    return op_name_ == other.op_name_
        && param_type_ids_ == other.param_type_ids_
        && param_is_static_ == other.param_is_static_;
  }

  /**
   * @brief Check whether this lite operation signature does not equal the other.
   *
   * @param other The other lite operation signature.
   * @return True if this signature does not equal the other, false otherwise.
   */
  bool operator!=(const OperationSignatureLite &r) const {
    return !(*this == r);
  }

  /**
   * @brief Check whether this lite operation signature can be coerced to the
   *        target lite signature, and return the coercion precedence.
   *
   * The coercion precedence defines a hierarchy that determines the implicit
   * type conversion sequence in operation overload resolution. More details
   * can be found in the documentation of OperationSignatureCoercisonPrecedence.
   *
   * @param The target lite operation signature.
   * @return The coercion precedence.
   */
  OperationSignatureCoercionPrecedence getCoercionPrecedence(
      const OperationSignatureLitePtr &target_signature) const;

  /**
   * @brief Get the hash code of this lite operation signature.
   * @note Two lite operation signatures are guaranteed to have the same hash
   *       code if they equal (==) each other.
   *
   * @return The hash code of this lite operation signature.
   */
  std::size_t getHash() const;

  /**
   * @brief Get a human readable representation of this lite operation signature.
   *
   * @return A human readable representation of this lite operation signature.
   */
  std::string toString() const;

  /**
   * @brief Create a lite operation signature.
   * @note This factory method assumes that all parameters are non-static.
   *
   * @param op_name The operation name.
   * @param param_type_ids The parameter type ids.
   * @return A lite operation signature.
   */
  static OperationSignatureLitePtr Create(const std::string &op_name,
                                          const std::vector<TypeID> &param_type_ids);

  /**
   * @brief Create a lite operation signature.
   *
   * @param op_name The operation name.
   * @param param_type_ids The parameter type ids.
   * @param param_is_static A bool vector indicating whether each parameter is
            static. It should have the same length as \p param_type_ids.
   * @return A lite operation signature.
   */
  static OperationSignatureLitePtr Create(const std::string &op_name,
                                          const std::vector<TypeID> &param_type_ids,
                                          const std::vector<bool> &param_is_static);

 private:
  // Count the number of static parameters.
  static std::size_t CountNumStaticParameters(const std::vector<bool> &param_is_static);

  OperationSignatureLite(const std::string &op_name,
                         const std::vector<TypeID> &param_type_ids,
                         const std::vector<bool> &param_is_static)
      : op_name_(ToLower(op_name)),
        param_type_ids_(param_type_ids),
        param_is_static_(param_is_static),
        num_static_params_(CountNumStaticParameters(param_is_static_)) {
    DCHECK_EQ(param_type_ids_.size(), param_is_static_.size());
  }

  const std::string op_name_;
  const std::vector<TypeID> param_type_ids_;
  const std::vector<bool> param_is_static_;
  const std::size_t num_static_params_;

  DISALLOW_COPY_AND_ASSIGN(OperationSignatureLite);
};

/**
 * @brief The equality functor for operation signature shared pointer.
 */
struct OperationSignatureLitePtrEqual {
  inline bool operator()(const OperationSignatureLitePtr &lhs,
                         const OperationSignatureLitePtr &rhs) const {
    return *lhs == *rhs;
  }
};

/**
 * @brief The hash functor for operation signature shared pointer.
 */
struct OperationSignatureLitePtrHash {
  inline std::size_t operator()(const OperationSignatureLitePtr &op_sig) const {
    return op_sig->getHash();
  }
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_OPERATION_SIGNATURE_LITE_HPP_
