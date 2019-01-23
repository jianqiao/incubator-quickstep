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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UTILITY_OPERATION_SYNTHESIZE_UTIL_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UTILITY_OPERATION_SYNTHESIZE_UTIL_HPP_

#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <type_traits>

#include "catalog/CatalogTypedefs.hpp"
#include "types/Type.hpp"
#include "types/TypedValue.hpp"
#include "types/containers/ColumnVector.hpp"
#include "utility/Macros.hpp"
#include "utility/meta/TypeList.hpp"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

template <typename ParameterTypeClass, bool nullable, typename Enable = void>
struct OperationParameterTraits;

template <typename ParameterTypeClass, bool nullable>
struct OperationParameterTraits<
    ParameterTypeClass, nullable,
    std::enable_if_t<ParameterTypeClass::kMemoryLayout == kCxxInlinePod>> {
  using TypeClass = ParameterTypeClass;
  using ColumnVectorType = NativeColumnVector;

  using ValueType = typename ParameterTypeClass::cpptype;
  using ConstPointer = const ValueType*;

  inline static ConstPointer GetValuePointer(const TypedValue &value) {
    if (nullable && value.isNull()) {
      return nullptr;
    }
    return static_cast<ConstPointer>(value.getDataPtr());
  }

  inline static ConstPointer GetValuePointer(const ColumnVectorType &column_vector,
                                             const std::size_t position) {
    return static_cast<ConstPointer>(
        column_vector.template getUntypedValue<nullable>(position));
  }

  template <typename Accessor>
  inline static ConstPointer GetValuePointer(const Accessor *accessor,
                                             const attribute_id attr_id) {
    return static_cast<ConstPointer>(
        accessor->template getUntypedValue<nullable>(attr_id));
  }

  inline static bool IsNull(ConstPointer value) {
    return value == nullptr;
  }
};

template <typename ParameterTypeClass, bool nullable>
struct OperationParameterTraits<
    ParameterTypeClass, nullable,
    std::enable_if_t<ParameterTypeClass::kMemoryLayout == kParInlinePod>> {
  using TypeClass = ParameterTypeClass;
  using ColumnVectorType = NativeColumnVector;

  using ValueType = void*;
  using ConstPointer = const void*;

  inline static ConstPointer GetValuePointer(const TypedValue &value) {
    if (nullable && value.isNull()) {
      return nullptr;
    }
    return value.getDataPtr();
  }

  inline static ConstPointer GetValuePointer(const ColumnVectorType &column_vector,
                                             const std::size_t position) {
    return column_vector.template getUntypedValue<nullable>(position);
  }

  template <typename Accessor>
  inline static ConstPointer GetValuePointer(const Accessor *accessor,
                                             const attribute_id attr_id) {
    return accessor->template getUntypedValue<nullable>(attr_id);
  }

  inline static bool IsNull(ConstPointer value) {
    return value == nullptr;
  }
};

template <typename ParameterTypeClass, bool nullable>
struct OperationParameterTraits<
    ParameterTypeClass, nullable,
    std::enable_if_t<ParameterTypeClass::kMemoryLayout == kParOutOfLinePod>> {
  using TypeClass = ParameterTypeClass;
  using ColumnVectorType = IndirectColumnVector;

  using ValueType = TypedValue;
  using ConstPointer = const TypedValue;

  inline static ConstPointer GetValuePointer(const TypedValue &value) {
    return value;
  }

  inline static ConstPointer GetValuePointer(const ColumnVectorType &column_vector,
                                             const std::size_t position) {
    return column_vector.getTypedValue(position);
  }

  template <typename Accessor>
  inline static ConstPointer GetValuePointer(const Accessor *accessor,
                                             const attribute_id attr_id) {
    return accessor->getTypedValue(attr_id);
  }

  inline static bool IsNull(const TypedValue &value) {
    return value.isNull();
  }
};


template <typename ResultTypeClass, typename Enable = void>
struct OperationResultTraits;

template <typename ResultTypeClass>
struct OperationResultTraits<
    ResultTypeClass,
    std::enable_if_t<ResultTypeClass::kMemoryLayout == kCxxInlinePod>> {
  using TypeClass = ResultTypeClass;
  using ColumnVectorType = NativeColumnVector;

  using ValueType = typename ResultTypeClass::cpptype;
  using ValuePointer = ValueType*;
  using ConstPointer = const ValueType*;
};

template <typename ResultTypeClass>
struct OperationResultTraits<
    ResultTypeClass,
    std::enable_if_t<ResultTypeClass::kMemoryLayout == kParInlinePod>> {
  using TypeClass = ResultTypeClass;
  using ColumnVectorType = NativeColumnVector;

  using ValueType = void*;
  using ValuePointer = void*;
  using ConstPointer = const void*;
};

template <typename ResultTypeClass>
struct OperationResultTraits<
    ResultTypeClass,
    std::enable_if_t<ResultTypeClass::kMemoryLayout == kParOutOfLinePod>> {
  using TypeClass = ResultTypeClass;
  using ColumnVectorType = IndirectColumnVector;

  using ValueType = TypedValue;
  using ValuePointer = TypedValue*;
  using ConstPointer = const TypedValue*;
};


template <typename Functor, typename Options>
class OperationFunctorAdapter {
 public:
  explicit OperationFunctorAdapter(const Functor &functor)
      : functor_(functor) {}

  template <typename ...FunctorCallArguments>
  inline auto operator()(FunctorCallArguments &&...args) const {
    return invokeInternal<Options, FunctorCallArguments...>(
        nullptr, std::forward<FunctorCallArguments>(args)...);
  }

 private:
  template <typename O, typename ...Ts>
  inline auto invokeInternal(std::enable_if_t<std::is_same<O, void>::value> *,
                             Ts &&...args) const {
    return functor_(std::forward<Ts>(args)...);
  }

  template <typename O, typename ...Ts>
  inline auto invokeInternal(std::enable_if_t<!std::is_same<O, void>::value> *,
                             Ts &&...args) const {
    return functor_.template operator()<O>(std::forward<Ts>(args)...);
  }

  const Functor &functor_;

  DISALLOW_COPY_AND_ASSIGN(OperationFunctorAdapter);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_OPERATION_UTILITY_SYNTHESIZE_UTIL_HPP_
