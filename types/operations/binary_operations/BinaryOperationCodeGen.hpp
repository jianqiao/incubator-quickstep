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

#ifndef QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_BINARY_OPERATION_CODE_GEN_HPP_
#define QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_BINARY_OPERATION_CODE_GEN_HPP_

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
#include "types/operations/binary_operations/BinaryOperation.hpp"
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
class UncheckedBinaryOperatorCodeGen : public UncheckedBinaryOperator {
 public:
  template <typename ...FunctorConstructorArguments>
  UncheckedBinaryOperatorCodeGen(const Type &left_type,
                                 const Type &right_type,
                                 const Type &result_type,
                                 FunctorConstructorArguments &&...func_args)
      : left_type_(left_type),
        right_type_(right_type),
        result_type_(result_type),
        functor_(std::forward<FunctorConstructorArguments>(func_args)...),
        adapter_(functor_) {
    DCHECK(left_type_.getTypeID() == LeftType::kStaticTypeID);
    DCHECK(right_type_.getTypeID() == RightType::kStaticTypeID);
    DCHECK(result_type_.getTypeID() == ResultType::kStaticTypeID);
  }

  TypedValue applyToTypedValues(const TypedValue &left,
                                const TypedValue &right) const override {
    if (argument_nullable && !kConsumesNull && (left.isNull() || right.isNull())) {
      return TypedValue(ResultType::kStaticTypeID);
    }
    return applyFunctorToValue<kResultLayout, kProducesNull>(
        LeftTraits::GetValuePointer(left), RightTraits::GetValuePointer(right));
  }

  ColumnVector* applyToColumnVectors(const ColumnVector &left,
                                     const ColumnVector &right) const override {
    using LeftCV = typename LeftTraits::ColumnVectorType;
    using RightCV = typename RightTraits::ColumnVectorType;
    using ResultCV = typename ResultTraits::ColumnVectorType;

    const LeftCV &left_cv = static_cast<const LeftCV&>(left);
    const RightCV &right_cv = static_cast<const RightCV&>(right);
    DCHECK_EQ(left_cv.size(), right_cv.size());

    std::unique_ptr<ResultCV> result_cv =
        std::make_unique<ResultCV>(result_type_, left_cv.size());

    for (std::size_t pos = 0; pos < left_cv.size(); ++pos) {
      LeftConstPointer left_value = LeftTraits::GetValuePointer(left_cv, pos);
      RightConstPointer right_value = RightTraits::GetValuePointer(right_cv, pos);
      if (argument_nullable && !kConsumesNull &&
          (LeftTraits::IsNull(left_value) || RightTraits::IsNull(right_value))) {
        result_cv->appendNullValue();
      } else {
        applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
            left_value, right_value, result_cv.get());
      }
    }
    return result_cv.release();
  }

  ColumnVector* applyToColumnVectorAndStaticValue(
      const ColumnVector &left,
      const TypedValue &right) const override {
    using ResultCV = typename ResultTraits::ColumnVectorType;
    using LeftCV = typename LeftTraits::ColumnVectorType;

    const LeftCV &left_cv = static_cast<const LeftCV&>(left);
    std::unique_ptr<ResultCV> result_cv =
        std::make_unique<ResultCV>(result_type_, left_cv.size());

    if (argument_nullable && !kConsumesNull && right.isNull()) {
      result_cv->fillWithNulls();
      return result_cv.release();
    }

    RightConstPointer right_value = RightTraits::GetValuePointer(right);

    for (std::size_t pos = 0; pos < left_cv.size(); ++pos) {
      LeftConstPointer left_value = LeftTraits::GetValuePointer(left_cv, pos);
      if (argument_nullable && !kConsumesNull && LeftTraits::IsNull(left_value)) {
        result_cv->appendNullValue();
      } else {
        applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
            left_value, right_value, result_cv.get());
      }
    }
    return result_cv.release();
  }

  ColumnVector* applyToStaticValueAndColumnVector(
      const TypedValue &left,
      const ColumnVector &right) const override {
    using ResultCV = typename ResultTraits::ColumnVectorType;
    using RightCV = typename RightTraits::ColumnVectorType;

    const RightCV &right_cv = static_cast<const RightCV&>(right);
    std::unique_ptr<ResultCV> result_cv =
        std::make_unique<ResultCV>(result_type_, right_cv.size());

    if (argument_nullable && !kConsumesNull && left.isNull()) {
      result_cv->fillWithNulls();
      return result_cv.release();
    }

    LeftConstPointer left_value = LeftTraits::GetValuePointer(left);

    for (std::size_t pos = 0; pos < right_cv.size(); ++pos) {
      RightConstPointer right_value = RightTraits::GetValuePointer(right_cv, pos);
      if (argument_nullable && !kConsumesNull && RightTraits::IsNull(right_value)) {
        result_cv->appendNullValue();
      } else {
        applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
            left_value, right_value, result_cv.get());
      }
    }
    return result_cv.release();
  }

#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION
  ColumnVector* applyToSingleValueAccessor(
      ValueAccessor *accessor,
      const attribute_id left_id,
      const attribute_id right_id) const override {
    return InvokeOnValueAccessorMaybeTupleIdSequenceAdapter(
        accessor,
        [&](auto *accessor) -> ColumnVector* {  // NOLINT(build/c++11)
      using ResultCV = typename ResultTraits::ColumnVectorType;
      std::unique_ptr<ResultCV> result_cv =
          std::make_unique<ResultCV>(result_type_, accessor->getNumTuples());

      accessor->beginIteration();
      while (accessor->next()) {
        LeftConstPointer left_value = LeftTraits::GetValuePointer(accessor, left_id);
        RightConstPointer right_value = RightTraits::GetValuePointer(accessor, right_id);
        if (argument_nullable && !kConsumesNull &&
            (LeftTraits::IsNull(left_value) || RightTraits::IsNull(right_value))) {
          result_cv->appendNullValue();
        } else {
          applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
              left_value, right_value, result_cv.get());
        }
      }
      return result_cv.release();
    });
  }

  ColumnVector* applyToValueAccessorAndStaticValue(
      ValueAccessor *left_accessor,
      const attribute_id left_id,
      const TypedValue &right) const override {
    return InvokeOnValueAccessorMaybeTupleIdSequenceAdapter(
        left_accessor,
        [&](auto *accessor) -> ColumnVector* {  // NOLINT(build/c++11)
      using ResultCV = typename ResultTraits::ColumnVectorType;
      std::unique_ptr<ResultCV> result_cv =
          std::make_unique<ResultCV>(result_type_, accessor->getNumTuples());

      if (argument_nullable && !kConsumesNull && right.isNull()) {
        result_cv->fillWithNulls();
        return result_cv.release();
      }

      RightConstPointer right_value = RightTraits::GetValuePointer(right);

      accessor->beginIteration();
      while (accessor->next()) {
        LeftConstPointer left_value = LeftTraits::GetValuePointer(accessor, left_id);
        if (argument_nullable && !kConsumesNull && LeftTraits::IsNull(left_value)) {
          result_cv->appendNullValue();
        } else {
          applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
              left_value, right_value, result_cv.get());
        }
      }
      return result_cv.release();
    });
  }

  ColumnVector* applyToStaticValueAndValueAccessor(
      const TypedValue &left,
      ValueAccessor *right_accessor,
      const attribute_id right_id) const override {
    return InvokeOnValueAccessorMaybeTupleIdSequenceAdapter(
        right_accessor,
        [&](auto *accessor) -> ColumnVector* {  // NOLINT(build/c++11)
      using ResultCV = typename ResultTraits::ColumnVectorType;
      std::unique_ptr<ResultCV> result_cv =
          std::make_unique<ResultCV>(result_type_, accessor->getNumTuples());

      if (argument_nullable && !kConsumesNull && left.isNull()) {
        result_cv->fillWithNulls();
        return result_cv.release();
      }

      LeftConstPointer left_value = LeftTraits::GetValuePointer(left);

      accessor->beginIteration();
      while (accessor->next()) {
        RightConstPointer right_value = RightTraits::GetValuePointer(accessor, right_id);
        if (argument_nullable && !kConsumesNull && RightTraits::IsNull(right_value)) {
          result_cv->appendNullValue();
        } else {
          applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
              left_value, right_value, result_cv.get());
        }
      }
      return result_cv.release();
    });
  }

  ColumnVector* applyToColumnVectorAndValueAccessor(
      const ColumnVector &left,
      ValueAccessor *right_accessor,
      const attribute_id right_id) const override {
    return InvokeOnValueAccessorMaybeTupleIdSequenceAdapter(
        right_accessor,
        [&](auto *accessor) -> ColumnVector* {  // NOLINT(build/c++11)
      using ResultCV = typename ResultTraits::ColumnVectorType;
      using LeftCV = typename LeftTraits::ColumnVectorType;

      const LeftCV &left_cv = static_cast<const LeftCV&>(left);
      DCHECK_EQ(left_cv.size(), accessor->getNumTuples());

      std::unique_ptr<ResultCV> result_cv =
          std::make_unique<ResultCV>(result_type_, left_cv.size());

      accessor->beginIteration();
      for (std::size_t pos = 0; pos < left_cv.size(); ++pos) {
        accessor->next();
        LeftConstPointer left_value = LeftTraits::GetValuePointer(left_cv, pos);
        RightConstPointer right_value = RightTraits::GetValuePointer(accessor, right_id);
        if (argument_nullable && !kConsumesNull &&
            (LeftTraits::IsNull(left_value) || RightTraits::IsNull(right_value))) {
          result_cv->appendNullValue();
        } else {
          applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
              left_value, right_value, result_cv.get());
        }
      }
      return result_cv.release();
    });
  }

  ColumnVector* applyToValueAccessorAndColumnVector(
      ValueAccessor *left_accessor,
      const attribute_id left_id,
      const ColumnVector &right) const override {
    return InvokeOnValueAccessorMaybeTupleIdSequenceAdapter(
        left_accessor,
        [&](auto *accessor) -> ColumnVector* {  // NOLINT(build/c++11)
      using ResultCV = typename ResultTraits::ColumnVectorType;
      using RightCV = typename RightTraits::ColumnVectorType;

      const RightCV &right_cv = static_cast<const RightCV&>(right);
      DCHECK_EQ(right_cv.size(), accessor->getNumTuples());

      std::unique_ptr<ResultCV> result_cv =
          std::make_unique<ResultCV>(result_type_, right_cv.size());

      accessor->beginIteration();
      for (std::size_t pos = 0; pos < right_cv.size(); ++pos) {
        accessor->next();
        LeftConstPointer left_value = LeftTraits::GetValuePointer(accessor, left_id);
        RightConstPointer right_value = RightTraits::GetValuePointer(right_cv, pos);
        if (argument_nullable && !kConsumesNull &&
            (LeftTraits::IsNull(left_value) || RightTraits::IsNull(right_value))) {
          result_cv->appendNullValue();
        } else {
          applyFunctorToValuesThenAppendToColumnVector<kResultLayout, kProducesNull>(
              left_value, right_value, result_cv.get());
        }
      }
      return result_cv.release();
    });
  }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION

  TypedValue accumulateColumnVector(
      const TypedValue &current,
      const ColumnVector &column_vector,
      std::size_t *num_tuples_applied) const override {
    LOG(FATAL) << "Not supported";
  }

#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION
  TypedValue accumulateValueAccessor(
      const TypedValue &current,
      ValueAccessor *accessor,
      const attribute_id value_accessor_id,
      std::size_t *num_tuples_applied) const override {
    LOG(FATAL) << "Not supported";
  }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION

 private:
  static constexpr bool kConsumesNull = Functor::kConsumesNullArgument;
  static constexpr bool kProducesNull = Functor::kProducesNullResult;

  using LeftType = typename Functor::LeftType;
  using RightType = typename Functor::RightType;
  using ResultType = typename Functor::ResultType;

  static constexpr MemoryLayout kResultLayout = ResultType::kMemoryLayout;

  using LeftTraits = OperationParameterTraits<LeftType, argument_nullable>;
  using RightTraits = OperationParameterTraits<RightType, argument_nullable>;
  using ResultTraits = OperationResultTraits<ResultType>;
  using LeftConstPointer = typename LeftTraits::ConstPointer;
  using RightConstPointer = typename RightTraits::ConstPointer;
  using ResultValueType = typename ResultTraits::ValueType;

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      LeftConstPointer left,
      RightConstPointer right,
      std::enable_if_t<layout == kCxxInlinePod && !produces_null> * = 0) const {
    ResultValueType result;
    adapter_(left, right, &result);
    return TypedValue(result);
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      LeftConstPointer left,
      RightConstPointer right,
      std::enable_if_t<layout == kCxxInlinePod && produces_null> * = 0) const {
    ResultValueType result;
    const bool is_null = adapter_(left, right, &result);
    return is_null ? TypedValue(ResultType::kStaticTypeID) : TypedValue(result);
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      LeftConstPointer left,
      RightConstPointer right,
      std::enable_if_t<layout == kParInlinePod && !produces_null> * = 0) const {
    const std::size_t len = result_type_.maximumByteLength();
    void *result = std::malloc(len);
    DCHECK(result != nullptr);
    adapter_(left, right, result);
    return TypedValue::CreateWithOwnedData(ResultType::kStaticTypeID, result, len);
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      LeftConstPointer left,
      RightConstPointer right,
      std::enable_if_t<layout == kParInlinePod && produces_null> * = 0) const {
    const std::size_t len = result_type_.maximumByteLength();
    void *result = std::malloc(len);
    DCHECK(result != nullptr);
    const bool is_null = adapter_(left, right, &result);
    if (is_null) {
      std::free(result);
      return TypedValue(ResultType::kStaticTypeID);
    } else {
      return TypedValue::CreateWithOwnedData(ResultType::kStaticTypeID, result, len);
    }
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      LeftConstPointer left,
      RightConstPointer right,
      std::enable_if_t<layout == kParOutOfLinePod && !produces_null> * = 0) const {
    TypedValue result;
    adapter_(left, right, &result);
    return result;
  }

  template <MemoryLayout layout, bool produces_null>
  inline TypedValue applyFunctorToValue(
      LeftConstPointer left,
      RightConstPointer right,
      std::enable_if_t<layout == kParOutOfLinePod && produces_null> * = 0) const {
    TypedValue result;
    const bool is_null = adapter_(left, right, &result);
    return is_null ? TypedValue(ResultType::kStaticTypeID) : result;
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValuesThenAppendToColumnVector(
      LeftConstPointer left,
      RightConstPointer right,
      ColumnVectorType *column_vector,
      std::enable_if_t<layout == kCxxInlinePod && !produces_null> * = 0) const {
    ResultValueType *result =
        static_cast<ResultValueType*>(column_vector->getPtrForDirectWrite());
    adapter_(left, right, result);
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValuesThenAppendToColumnVector(
      LeftConstPointer left,
      RightConstPointer right,
      ColumnVectorType *column_vector,
      std::enable_if_t<layout == kCxxInlinePod && produces_null> * = 0) const {
    ResultValueType *result =
        static_cast<ResultValueType*>(column_vector->getPtrForDirectWrite());
    const bool is_null = adapter_(left, right, result);
    if (is_null) {
      column_vector->setNullValue(column_vector->sizeInl() - 1);
    }
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValuesThenAppendToColumnVector(
      LeftConstPointer left,
      RightConstPointer right,
      ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParInlinePod && !produces_null> * = 0) const {
    adapter_(left, right, column_vector->getPtrForDirectWrite());
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValuesThenAppendToColumnVector(
      LeftConstPointer left,
      RightConstPointer right,
      ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParInlinePod && produces_null> * = 0) const {
    const bool is_null = adapter_(left, right, column_vector->getPtrForDirectWrite());
    if (is_null) {
      column_vector->setNullValue(column_vector->sizeInl() - 1);
    }
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValuesThenAppendToColumnVector(
      LeftConstPointer left,
      RightConstPointer right,
      ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParOutOfLinePod && !produces_null> * = 0) const {
    TypedValue result;
    adapter_(left, right, &result);
    column_vector->appendTypedValue(std::move(result));
  }

  template <MemoryLayout layout, bool produces_null, typename ColumnVectorType>
  inline void applyFunctorToValuesThenAppendToColumnVector(
      LeftConstPointer left,
      RightConstPointer right,
      ColumnVectorType *column_vector,
      std::enable_if_t<layout == kParOutOfLinePod && produces_null> * = 0) const {
    TypedValue result;
    const bool is_null = adapter_(left, right, &result);
    if (is_null) {
      column_vector->appendNullValue();
    } else {
      column_vector->appendTypedValue(std::move(result));
    }
  }

  const Type &left_type_;
  const Type &right_type_;
  const Type &result_type_;
  const Functor functor_;
  const OperationFunctorAdapter<Functor, Options> adapter_;

  DISALLOW_COPY_AND_ASSIGN(UncheckedBinaryOperatorCodeGen);
};


template <typename Functor>
class BinaryOperationCodeGen : public BinaryOperation {
 public:
  BinaryOperationCodeGen()
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
        OperationSignatureLite::Create(
            getShortName(), {LeftType::kStaticTypeID, RightType::kStaticTypeID});
    return {signature};
  }

  std::size_t getOperatorPrecedence() const override {
    return op_precedence_;
  }

  bool canApplyToSignature(const OperationSignaturePtr &signature,
                           std::string *diagnostic_message) const override {
    DCHECK_EQ(2u, signature->getNumParameters());
    DCHECK_EQ(LeftType::kStaticTypeID, signature->getParameterType(0).getTypeID());
    DCHECK_EQ(RightType::kStaticTypeID, signature->getParameterType(1).getTypeID());
    return true;
  }

  const Type& resultTypeForSignature(
      const OperationSignaturePtr &signature) const override {
    DCHECK(BinaryOperation::canApplyToSignature(signature));
    return resultTypeForSignatureInternal<
        FunctorDefinesResultType<Functor>::value>(signature);
  }

  UncheckedBinaryOperator* makeUncheckedBinaryOperatorForSignature(
      const OperationSignaturePtr &signature) const override {
    DCHECK(BinaryOperation::canApplyToSignature(signature));
    return makeUncheckedBinaryOperatorInternal<
        std::is_default_constructible<Functor>::value>(signature);
  }

 private:
  using LeftType = typename Functor::LeftType;
  using RightType = typename Functor::RightType;
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
  inline UncheckedBinaryOperator* makeUncheckedBinaryOperatorInternal(
      const OperationSignaturePtr &signature,
      std::enable_if_t<functor_uses_default_constructor> * = 0) const {
    const Type& left_type = signature->getParameterType(0);
    const Type& right_type = signature->getParameterType(1);
    return meta::BoolDispatcher::InvokeOn(
        left_type.isNullable() || right_type.isNullable(),
        [&](auto typelist) -> UncheckedBinaryOperator* {  // NOLINT(build/c++11)
      constexpr bool argument_nullable = decltype(typelist)::head::value;
      return new UncheckedBinaryOperatorCodeGen<Functor, argument_nullable>(
          left_type, right_type, resultTypeForSignature(signature));
    });
  }

  template <bool functor_uses_default_constructor>
  inline UncheckedBinaryOperator* makeUncheckedBinaryOperatorInternal(
      const OperationSignaturePtr &signature,
      std::enable_if_t<!functor_uses_default_constructor> * = 0) const {
    const Type& left_type = signature->getParameterType(0);
    const Type& right_type = signature->getParameterType(1);
    return meta::BoolDispatcher::InvokeOn(
        left_type.isNullable() || right_type.isNullable(),
        [&](auto typelist) -> UncheckedBinaryOperator* {  // NOLINT(build/c++11)
      constexpr bool argument_nullable = decltype(typelist)::head::value;
      return new UncheckedBinaryOperatorCodeGen<Functor, argument_nullable>(
          left_type, right_type, resultTypeForSignature(signature), signature);
    });
  }

  const std::string op_name_;
  const std::string op_short_name_;
  const std::size_t op_precedence_;

  DISALLOW_COPY_AND_ASSIGN(BinaryOperationCodeGen);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_OPERATIONS_BINARY_OPERATIONS_BINARY_OPERATION_CODE_GEN_HPP_
