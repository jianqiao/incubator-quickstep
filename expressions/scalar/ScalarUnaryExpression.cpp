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

#include "expressions/scalar/ScalarUnaryExpression.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "catalog/CatalogTypedefs.hpp"
#include "expressions/Expressions.pb.h"
#include "storage/ValueAccessor.hpp"
#include "types/Type.hpp"
#include "types/TypeErrors.hpp"
#include "types/TypedValue.hpp"
#include "types/containers/ColumnVector.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/Operation.pb.h"
#include "types/operations/unary_operations/UnaryOperation.hpp"

namespace quickstep {

struct SubBlocksReference;

ScalarUnaryExpression::ScalarUnaryExpression(const OperationSignaturePtr &signature,
                                             const UnaryOperation &operation,
                                             Scalar *operand)
    : Scalar(operation.resultTypeForSignature(signature)),
      signature_(signature),
      operation_(operation),
      operand_(operand) {
  DCHECK(operation_.canApplyToSignature(signature_));
  fast_operator_.reset(operation_.makeUncheckedUnaryOperatorForSignature(signature_));
  if (operand_->hasStaticValue()) {
    static_value_ = std::make_unique<TypedValue>(
        fast_operator_->applyToTypedValue(operand_->getStaticValue()));
    static_value_->ensureNotReference();
  }
}

serialization::Scalar ScalarUnaryExpression::getProto() const {
  serialization::Scalar proto;
  proto.set_data_source(serialization::Scalar::UNARY_EXPRESSION);
  proto.MutableExtension(serialization::ScalarUnaryExpression::signature)
      ->MergeFrom(signature_->getProto());
  proto.MutableExtension(serialization::ScalarUnaryExpression::operand)
      ->MergeFrom(operand_->getProto());
  return proto;
}

Scalar* ScalarUnaryExpression::clone() const {
  return new ScalarUnaryExpression(signature_, operation_, operand_->clone());
}

TypedValue ScalarUnaryExpression::getValueForSingleTuple(const ValueAccessor &accessor,
                                                         const tuple_id tuple) const {
  if (static_value_ != nullptr) {
    return static_value_->makeReferenceToThis();
  } else {
    return fast_operator_->applyToTypedValue(
        operand_->getValueForSingleTuple(accessor, tuple));
  }
}

TypedValue ScalarUnaryExpression::getValueForJoinedTuples(
    const ValueAccessor &left_accessor,
    const tuple_id left_tuple_id,
    const ValueAccessor &right_accessor,
    const tuple_id right_tuple_id) const {
  if (static_value_ != nullptr) {
    return static_value_->makeReferenceToThis();
  } else {
    return fast_operator_->applyToTypedValue(
        operand_->getValueForJoinedTuples(left_accessor,
                                          left_tuple_id,
                                          right_accessor,
                                          right_tuple_id));
  }
}

ColumnVectorPtr ScalarUnaryExpression::getAllValues(
    ValueAccessor *accessor,
    const SubBlocksReference *sub_blocks_ref,
    ColumnVectorCache *cv_cache) const {
  if (static_value_ != nullptr) {
    return ColumnVectorPtr(
        ColumnVector::MakeVectorOfValue(getType(),
                                        *static_value_,
                                        accessor->getNumTuplesVirtual()));
  } else {
#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION
    const attribute_id operand_attr_id = operand_->getAttributeIdForValueAccessor();
    if (operand_attr_id != -1) {
      return ColumnVectorPtr(
          fast_operator_->applyToValueAccessor(accessor, operand_attr_id));
    }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION

    ColumnVectorPtr operand_result(
        operand_->getAllValues(accessor, sub_blocks_ref, cv_cache));
    return ColumnVectorPtr(
        fast_operator_->applyToColumnVector(*operand_result));
  }
}

ColumnVectorPtr ScalarUnaryExpression::getAllValuesForJoin(
    ValueAccessor *left_accessor,
    ValueAccessor *right_accessor,
    const std::vector<std::pair<tuple_id, tuple_id>> &joined_tuple_ids,
    ColumnVectorCache *cv_cache) const {
  if (static_value_ != nullptr) {
    return ColumnVectorPtr(
        ColumnVector::MakeVectorOfValue(getType(),
                                        *static_value_,
                                        joined_tuple_ids.size()));
  } else {
    ColumnVectorPtr operand_result(
        operand_->getAllValuesForJoin(left_accessor,
                                      right_accessor,
                                      joined_tuple_ids,
                                      cv_cache));
    return ColumnVectorPtr(
        fast_operator_->applyToColumnVector(*operand_result));
  }
}

void ScalarUnaryExpression::getFieldStringItems(
    std::vector<std::string> *inline_field_names,
    std::vector<std::string> *inline_field_values,
    std::vector<std::string> *non_container_child_field_names,
    std::vector<const Expression*> *non_container_child_fields,
    std::vector<std::string> *container_child_field_names,
    std::vector<std::vector<const Expression*>> *container_child_fields) const {
  Scalar::getFieldStringItems(inline_field_names,
                              inline_field_values,
                              non_container_child_field_names,
                              non_container_child_fields,
                              container_child_field_names,
                              container_child_fields);

  if (static_value_ != nullptr) {
    inline_field_names->emplace_back("static_value");
    if (static_value_->isNull()) {
      inline_field_values->emplace_back("NULL");
    } else {
      inline_field_values->emplace_back(type_.printValueToString(*static_value_));
    }
  }

  inline_field_names->emplace_back("signature");
  inline_field_values->emplace_back(signature_->toString());

  non_container_child_field_names->emplace_back("operand");
  non_container_child_fields->emplace_back(operand_.get());
}

}  // namespace quickstep
