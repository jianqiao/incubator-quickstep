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

#ifndef QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_OPERATION_CODE_GEN_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_OPERATION_CODE_GEN_HPP_

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "catalog/CatalogTypedefs.hpp"
#include "storage/ValueAccessor.hpp"
#include "storage/ValueAccessorUtil.hpp"
#include "types/Type.hpp"
#include "types/TypeFactory.hpp"
#include "types/TypeID.hpp"
#include "types/TypedValue.hpp"
#include "types/containers/ColumnVector.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperationSignatureLite.hpp"
#include "types/operations/OperationTraits.hpp"
#include "types/operations/unary_operations/UnaryOperation.hpp"
#include "utility/meta/Common.hpp"
#include "utility/meta/MultipleDispatcher.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Types
 *  @{
 */

/**
 * @brief ... \p Options has user-defined semantics ...
 */
template <typename Functor, bool argument_nullable, typename Options = void>
class UncheckedUnaryOperatorCodeGen : public UncheckedUnaryOperator {
 public:
  template <typename ...FunctorConstructorArguments>
  UncheckedUnaryOperatorCodeGen(const Type &argument_type,
                                const Type &result_type,
                                FunctorConstructorArguments &&...func_args)
      : argument_type_(argument_type),
        result_type_(result_type),
        functor_(std::forward<FunctorConstructorArguments>(func_args)...),
        adapter_(functor_) {
    DCHECK(argument_type.getTypeID() == ParameterType::kStaticTypeID);
    DCHECK(result_type.getTypeID() == ResultType::kStaticTypeID);
  }

  TypedValue applyToTypedValue(const TypedValue &argument) const override {
    if (argument_nullable && !kConsumesNull && argument.isNull()) {
      return TypedValue(ResultType::kStaticTypeID);
    }
    return applyFunctorToValue<kResultLayout, kProducesNull>(
        ParameterTraits::GetValuePointer(argument));
  }

  ColumnVector* applyToColumnVector(const ColumnVector &argument) const override {
    using ArgumentCV = typename ParameterTraits::ColumnVectorType;
    using ResultCV = typename ResultTraits::ColumnVectorType;

    const ArgumentCV &argument_cv = static_cast<const ArgumentCV&>(argument);
    std::unique_ptr<ResultCV> result_cv =
        std::make_unique<ResultCV>(result_type_, argument_cv.size());

    for (std::size_t pos = 0; pos < argument_cv.size(); ++pos) {
      ConstPointer value = ParameterTraits::GetValuePointer(argument_cv, pos);
      if (argument_nullable && !kConsumesNull && ParameterTraits::IsNull(value)) {
        result_cv->appendNullValue();
      } else {
        applyFunctorToValueThenAppendToColumnVector<
            kResultLayout, kProducesNull>(value, result_cv.get());
      }
    }
    return result_cv.release();
  }

  ColumnVector* applyToValueAccessor(ValueAccessor *accessor,
                                     const attribute_id attr_id) const override {
    using ResultCV = typename ResultTraits::ColumnVectorType;
    return InvokeOnValueAccessorMaybeTupleIdSequenceAdapter(
        accessor,
        [&](auto *accessor) -> ColumnVector* {  // NOLINT(build/c++11)
      std::unique_ptr<ResultCV> result_cv =
          std::make_unique<ResultCV>(result_type_, accessor->getNumTuples());

      accessor->beginIteration();
      while (accessor->next()) {
        ConstPointer value = ParameterTraits::GetValuePointer(accessor, attr_id);
        if (argument_nullable && !kConsumesNull && ParameterTraits::IsNull(value)) {
          result_cv->appendNullValue();
        } else {
          applyFunctorToValueThenAppendToColumnVector<
              kResultLayout, kProducesNull>(value, result_cv.get());
        }
      }
      return result_cv.release();
    });
  }

 private:
  static constexpr bool kConsumesNull = Functor::kConsumesNullArgument;
  static constexpr bool kProducesNull = Functor::kProducesNullResult;

  using ParameterType = typename Functor::ParameterType;
  using ResultType = typename Functor::ResultType;

  static constexpr MemoryLayout kResultLayout = ResultType::kMemoryLayout;

  using ParameterTraits = OperationParameterTraits<ParameterType, argument_nullable>;
  using ResultTraits = OperationResultTraits<ResultType>;
  using ConstPointer = typename ParameterTraits::ConstPointer;
  using ResultValueType = typename ResultTraits::ValueType;

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      ConstPointer value,
      std::enable_if_t<layout == kCxxInlinePod && !produces_null> * = 0) const {
    ResultValueType result;
    adapter_(value, &result);
    return TypedValue(result);
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      ConstPointer value,
      std::enable_if_t<layout == kCxxInlinePod && produces_null> * = 0) const {
    ResultValueType result;
    const bool is_null = adapter_(value, &result);
    return is_null ? TypedValue(ResultType::kStaticTypeID) : TypedValue(result);
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      ConstPointer value,
      std::enable_if_t<layout == kParInlinePod && !produces_null> * = 0) const {
    const std::size_t len = result_type_.maximumByteLength();
    void *result = std::malloc(len);
    DCHECK(result != nullptr);
    adapter_(value, result);
    return TypedValue::CreateWithOwnedData(ResultType::kStaticTypeID, result, len);
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      ConstPointer value,
      std::enable_if_t<layout == kParInlinePod && produces_null> * = 0) const {
    const std::size_t len = result_type_.maximumByteLength();
    void *result = std::malloc(len);
    DCHECK(result != nullptr);
    const bool is_null = adapter_(value, &result);
    if (is_null) {
      std::free(result);
      return TypedValue(ResultType::kStaticTypeID);
    } else {
      return TypedValue::CreateWithOwnedData(ResultType::kStaticTypeID, result, len);
    }
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      ConstPointer value,
      std::enable_if_t<layout == kParOutOfLinePod && !produces_null> * = 0) const {
    TypedValue result;
    adapter_(value, &result);
    return result;
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      ConstPointer value,
      std::enable_if_t<layout == kParOutOfLinePod && produces_null> * = 0) const {
    TypedValue result;
    const bool is_null = adapter_(value, &result);
    return is_null ? TypedValue(ResultType::kStaticTypeID) : result;
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValueThenAppendToColumnVector(
      ConstPointer value, ColumnVectorType *column_vector,
      std::enable_if_t<layout == kCxxInlinePod && !produces_null> * = 0) const {
    ResultValueType *result =
        static_cast<ResultValueType*>(column_vector->getPtrForDirectWrite());
    adapter_(value, result);
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValueThenAppendToColumnVector(
      ConstPointer value, ColumnVectorType *column_vector,
      std::enable_if_t<layout == kCxxInlinePod && produces_null> * = 0) const {
    ResultValueType *result =
        static_cast<ResultValueType*>(column_vector->getPtrForDirectWrite());
    const bool is_null = adapter_(value, result);
    if (is_null) {
      column_vector->setNullValue(column_vector->sizeInl() - 1);
    }
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValueThenAppendToColumnVector(
      ConstPointer value, ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParInlinePod && !produces_null> * = 0) const {
    adapter_(value, column_vector->getPtrForDirectWrite());
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValueThenAppendToColumnVector(
      ConstPointer value, ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParInlinePod && produces_null> * = 0) const {
    const bool is_null = adapter_(value, column_vector->getPtrForDirectWrite());
    if (is_null) {
      column_vector->setNullValue(column_vector->sizeInl() - 1);
    }
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValueThenAppendToColumnVector(
      ConstPointer value, ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParOutOfLinePod && !produces_null> * = 0) const {
    TypedValue result;
    adapter_(value, &result);
    column_vector->appendTypedValue(std::move(result));
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValueThenAppendToColumnVector(
      ConstPointer value, ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParOutOfLinePod && produces_null> * = 0) const {
    TypedValue result;
    const bool is_null = adapter_(value, &result);
    if (is_null) {
      column_vector->appendNullValue();
    } else {
      column_vector->appendTypedValue(std::move(result));
    }
  }

  const Type &argument_type_;
  const Type &result_type_;
  const Functor functor_;
  const OperationFunctorAdapter<Functor, Options> adapter_;

  DISALLOW_COPY_AND_ASSIGN(UncheckedUnaryOperatorCodeGen);
};


template <typename Functor>
class UnaryOperationCodeGen : public UnaryOperation {
 public:
  UnaryOperationCodeGen()
      : op_name_(Functor::GetName()),
        op_short_name_(Functor::GetShortName()),
        op_precedence_(Functor::GetOperatorPrecedence()) {
  }

  std::string getName() const override {
    return op_name_;
  }

  std::string getShortName() const override {
    return op_short_name_;
  }

  std::vector<OperationSignatureLitePtr> getSignatures() const override {
    const OperationSignatureLitePtr signature =
        OperationSignatureLite::Create(getShortName(),
                                       {ParameterType::kStaticTypeID});
    return {signature};
  }

  std::size_t getOperatorPrecedence() const override {
    return op_precedence_;
  }

  bool canApplyToSignature(const OperationSignaturePtr &signature,
                           std::string *diagnostic_message) const override {
    DCHECK_EQ(1u, signature->getNumParameters());
    DCHECK_EQ(ParameterType::kStaticTypeID, signature->getParameterType(0).getTypeID());
    return true;
  }

  const Type& resultTypeForSignature(
      const OperationSignaturePtr &signature) const override {
    DCHECK(UnaryOperation::canApplyToSignature(signature));
    return resultTypeForSignatureInternal<
        FunctorDefinesResultType<Functor>::value>(signature);
  }

  UncheckedUnaryOperator* makeUncheckedUnaryOperatorForSignature(
      const OperationSignaturePtr &signature) const override {
    DCHECK(UnaryOperation::canApplyToSignature(signature));
    return makeUncheckedUnaryOperatorInternal<
        std::is_default_constructible<Functor>::value>(signature);
  }

 private:
  using ParameterType = typename Functor::ParameterType;
  using ResultType = typename Functor::ResultType;

  QUICKSTEP_TRAIT_HAS_STATIC_METHOD(FunctorDefinesResultType, ResultTypeForSignature);

  template <bool functor_defines_result_type>
  inline const Type& resultTypeForSignatureInternal(
      const OperationSignaturePtr &signature,
      std::enable_if_t<!functor_defines_result_type &&
                       ResultType::kMemoryLayout == kCxxInlinePod> * = 0) const {
    return TypeFactory::GetType(ResultType::kStaticTypeID,
                                signature->getParameterType(0).isNullable());
  }

  template <bool functor_defines_result_type>
  inline const Type& resultTypeForSignatureInternal(
      const OperationSignaturePtr &signature,
      std::enable_if_t<functor_defines_result_type  ||
                       ResultType::kMemoryLayout == kParInlinePod ||
                       ResultType::kMemoryLayout == kParOutOfLinePod> * = 0) const {
    return Functor::ResultTypeForSignature(signature);
  }

  template <bool functor_uses_default_constructor>
  inline UncheckedUnaryOperator* makeUncheckedUnaryOperatorInternal(
      const OperationSignaturePtr &signature,
      std::enable_if_t<functor_uses_default_constructor> * = 0) const {
    const Type& argument_type = signature->getParameterType(0);
    return meta::BoolDispatcher::InvokeOn(
        argument_type.isNullable(),
        [&](auto typelist) -> UncheckedUnaryOperator* {  // NOLINT(build/c++11)
      constexpr bool argument_nullable = decltype(typelist)::head::value;
      return new UncheckedUnaryOperatorCodeGen<Functor, argument_nullable>(
          argument_type, resultTypeForSignature(signature));
    });
  }

  template <bool functor_uses_default_constructor>
  inline UncheckedUnaryOperator* makeUncheckedUnaryOperatorInternal(
      const OperationSignaturePtr &signature,
      std::enable_if_t<!functor_uses_default_constructor> * = 0) const {
    const Type& argument_type = signature->getParameterType(0);
    return meta::BoolDispatcher::InvokeOn(
        argument_type.isNullable(),
        [&](auto typelist) -> UncheckedUnaryOperator* {  // NOLINT(build/c++11)
      constexpr bool argument_nullable = decltype(typelist)::head::value;
      return new UncheckedUnaryOperatorCodeGen<Functor, argument_nullable>(
          argument_type, resultTypeForSignature(signature), signature);
    });
  }

  const std::string op_name_;
  const std::string op_short_name_;
  const std::size_t op_precedence_;

  DISALLOW_COPY_AND_ASSIGN(UnaryOperationCodeGen);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_UNARY_OPERATIONS_UNARY_OPERATION_CODE_GEN_HPP_
