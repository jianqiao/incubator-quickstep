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

#include "expressions/scalar/ScalarBinaryExpression.hpp"

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
#include "types/operations/binary_operations/BinaryOperation.hpp"

#include "glog/logging.h"

namespace quickstep {

ScalarBinaryExpression::ScalarBinaryExpression(const OperationSignaturePtr &signature,
                                               const BinaryOperation &operation,
                                               Scalar *left_operand,
                                               Scalar *right_operand)
    : Scalar(operation.resultTypeForSignature(signature)),
      signature_(signature),
      operation_(operation),
      left_operand_(left_operand),
      right_operand_(right_operand) {
  DCHECK(operation_.canApplyToSignature(signature_));
  fast_operator_.reset(operation_.makeUncheckedBinaryOperatorForSignature(signature_));
  if (left_operand_->hasStaticValue() && right_operand_->hasStaticValue()) {
    static_value_ = std::make_unique<TypedValue>(
        fast_operator_->applyToTypedValues(left_operand_->getStaticValue(),
                                           right_operand_->getStaticValue()));
    static_value_->ensureNotReference();
  }
}

serialization::Scalar ScalarBinaryExpression::getProto() const {
  serialization::Scalar proto;
  proto.set_data_source(serialization::Scalar::BINARY_EXPRESSION);
  proto.MutableExtension(serialization::ScalarBinaryExpression::signature)
      ->MergeFrom(signature_->getProto());
  proto.MutableExtension(serialization::ScalarBinaryExpression::left_operand)
      ->MergeFrom(left_operand_->getProto());
  proto.MutableExtension(serialization::ScalarBinaryExpression::right_operand)
      ->MergeFrom(right_operand_->getProto());
  return proto;
}

Scalar* ScalarBinaryExpression::clone() const {
  return new ScalarBinaryExpression(signature_,
                                    operation_,
                                    left_operand_->clone(),
                                    right_operand_->clone());
}

TypedValue ScalarBinaryExpression::getValueForSingleTuple(const ValueAccessor &accessor,
                                                          const tuple_id tuple) const {
  if (fast_operator_.get() == nullptr) {
    return static_value_->makeReferenceToThis();
  } else {
    return fast_operator_->applyToTypedValues(left_operand_->getValueForSingleTuple(accessor, tuple),
                                              right_operand_->getValueForSingleTuple(accessor, tuple));
  }
}

TypedValue ScalarBinaryExpression::getValueForJoinedTuples(
    const ValueAccessor &left_accessor,
    const tuple_id left_tuple_id,
    const ValueAccessor &right_accessor,
    const tuple_id right_tuple_id) const {
  if (fast_operator_.get() == nullptr) {
    return static_value_->makeReferenceToThis();
  } else {
    return fast_operator_->applyToTypedValues(
        left_operand_->getValueForJoinedTuples(left_accessor,
                                               left_tuple_id,
                                               right_accessor,
                                               right_tuple_id),
        right_operand_->getValueForJoinedTuples(left_accessor,
                                                left_tuple_id,
                                                right_accessor,
                                                right_tuple_id));
  }
}

ColumnVectorPtr ScalarBinaryExpression::getAllValues(
    ValueAccessor *accessor,
    const SubBlocksReference *sub_blocks_ref,
    ColumnVectorCache *cv_cache) const {
  if (fast_operator_.get() == nullptr) {
    return ColumnVectorPtr(
        ColumnVector::MakeVectorOfValue(getType(),
                                        *static_value_,
                                        accessor->getNumTuplesVirtual()));
  } else {
    // NOTE(chasseur): We don't check if BOTH operands have a static value,
    // because if they did then this expression would also have a static value
    // handled in the case above.
    if (left_operand_->hasStaticValue()) {
#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION
      const attribute_id right_operand_attr_id
          = right_operand_->getAttributeIdForValueAccessor();
      if (right_operand_attr_id != -1) {
        return ColumnVectorPtr(
            fast_operator_->applyToStaticValueAndValueAccessor(
                left_operand_->getStaticValue(),
                accessor,
                right_operand_attr_id));
      }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION

      ColumnVectorPtr right_result(
          right_operand_->getAllValues(accessor, sub_blocks_ref, cv_cache));
      return ColumnVectorPtr(
          fast_operator_->applyToStaticValueAndColumnVector(
              left_operand_->getStaticValue(),
              *right_result));
    } else if (right_operand_->hasStaticValue()) {
#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION
      const attribute_id left_operand_attr_id
          = left_operand_->getAttributeIdForValueAccessor();
      if (left_operand_attr_id != -1) {
        return ColumnVectorPtr(
            fast_operator_->applyToValueAccessorAndStaticValue(
                accessor,
                left_operand_attr_id,
                right_operand_->getStaticValue()));
      }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION

      ColumnVectorPtr left_result(
          left_operand_->getAllValues(accessor, sub_blocks_ref, cv_cache));
      return ColumnVectorPtr(
          fast_operator_->applyToColumnVectorAndStaticValue(
              *left_result,
              right_operand_->getStaticValue()));
    } else {
#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION
      const attribute_id left_operand_attr_id
          = left_operand_->getAttributeIdForValueAccessor();
      const attribute_id right_operand_attr_id
          = right_operand_->getAttributeIdForValueAccessor();

      if (left_operand_attr_id != -1) {
        if (right_operand_attr_id != -1) {
          return ColumnVectorPtr(
              fast_operator_->applyToSingleValueAccessor(
                  accessor,
                  left_operand_attr_id,
                  right_operand_attr_id));
        } else {
          ColumnVectorPtr right_result(
              right_operand_->getAllValues(accessor, sub_blocks_ref, cv_cache));
          return ColumnVectorPtr(
              fast_operator_->applyToValueAccessorAndColumnVector(
                  accessor,
                  left_operand_attr_id,
                  *right_result));
        }
      } else if (right_operand_attr_id != -1) {
        ColumnVectorPtr left_result(
            left_operand_->getAllValues(accessor, sub_blocks_ref, cv_cache));
        return ColumnVectorPtr(
            fast_operator_->applyToColumnVectorAndValueAccessor(
                *left_result,
                accessor,
                right_operand_attr_id));
      }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_SELECTION

      ColumnVectorPtr left_result(
          left_operand_->getAllValues(accessor, sub_blocks_ref, cv_cache));
      ColumnVectorPtr right_result(
          right_operand_->getAllValues(accessor, sub_blocks_ref, cv_cache));
      return ColumnVectorPtr(
          fast_operator_->applyToColumnVectors(*left_result, *right_result));
    }
  }
}

ColumnVectorPtr ScalarBinaryExpression::getAllValuesForJoin(
    ValueAccessor *left_accessor,
    ValueAccessor *right_accessor,
    const std::vector<std::pair<tuple_id, tuple_id>> &joined_tuple_ids,
    ColumnVectorCache *cv_cache) const {
  if (fast_operator_.get() == nullptr) {
    return ColumnVectorPtr(
        ColumnVector::MakeVectorOfValue(getType(),
                                        *static_value_,
                                        joined_tuple_ids.size()));
  } else {
    if (left_operand_->hasStaticValue()) {
#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN
      const attribute_id right_operand_attr_id
          = right_operand_->getAttributeIdForValueAccessor();
      if (right_operand_attr_id != -1) {
        const JoinSide join_side = right_operand_->join_side();
        DCHECK(join_side != kNone);
        const bool using_left_relation = (join_side == kLeftSide);
        ValueAccessor *right_operand_accessor = using_left_relation ? left_accessor
                                                                    : right_accessor;
        return ColumnVectorPtr(
            fast_operator_->applyToStaticValueAndValueAccessorForJoin(
                left_operand_->getStaticValue(),
                right_operand_accessor,
                using_left_relation,
                right_operand_attr_id,
                joined_tuple_ids));
      }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN

      ColumnVectorPtr right_result(
          right_operand_->getAllValuesForJoin(left_accessor,
                                              right_accessor,
                                              joined_tuple_ids,
                                              cv_cache));
      return ColumnVectorPtr(
          fast_operator_->applyToStaticValueAndColumnVector(
              left_operand_->getStaticValue(),
              *right_result));
    } else if (right_operand_->hasStaticValue()) {
#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN
      const attribute_id left_operand_attr_id
          = left_operand_->getAttributeIdForValueAccessor();
      if (left_operand_attr_id != -1) {
        const JoinSide join_side = left_operand_->join_side();
        DCHECK(join_side != kNone);
        const bool using_left_relation = (join_side == kLeftSide);
        ValueAccessor *left_operand_accessor = using_left_relation ? left_accessor
                                                                   : right_accessor;
        return ColumnVectorPtr(
            fast_operator_->applyToValueAccessorAndStaticValueForJoin(
                left_operand_accessor,
                using_left_relation,
                left_operand_attr_id,
                right_operand_->getStaticValue(),
                joined_tuple_ids));
      }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN

      ColumnVectorPtr left_result(
          left_operand_->getAllValuesForJoin(left_accessor,
                                             right_accessor,
                                             joined_tuple_ids,
                                             cv_cache));
      return ColumnVectorPtr(
          fast_operator_->applyToColumnVectorAndStaticValue(
              *left_result,
              right_operand_->getStaticValue()));
    } else {
#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN
      const attribute_id left_operand_attr_id
          = left_operand_->getAttributeIdForValueAccessor();
      const attribute_id right_operand_attr_id
          = right_operand_->getAttributeIdForValueAccessor();
      if (left_operand_attr_id != -1) {
        const JoinSide join_side = left_operand_->join_side();
        DCHECK(join_side != kNone);
        const bool using_left_relation_for_left_operand = (join_side == kLeftSide);
        ValueAccessor *left_operand_accessor = using_left_relation_for_left_operand ? left_accessor
                                                                                    : right_accessor;

#ifdef QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN_WITH_BINARY_EXPRESSIONS
        if (right_operand_attr_id != -1) {
          const JoinSide join_side = right_operand_->join_side();
          DCHECK(join_side != kNone);
          const bool using_left_relation_for_right_operand = (join_side == kLeftSide);
          ValueAccessor *right_operand_accessor = using_left_relation_for_right_operand ? left_accessor
                                                                                        : right_accessor;
          return ColumnVectorPtr(
              fast_operator_->applyToValueAccessorsForJoin(left_operand_accessor,
                                                           using_left_relation_for_left_operand,
                                                           left_operand_attr_id,
                                                           right_operand_accessor,
                                                           using_left_relation_for_right_operand,
                                                           right_operand_attr_id,
                                                           joined_tuple_ids));
        }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN_WITH_BINARY_EXPRESSIONS
        ColumnVectorPtr right_result(
            right_operand_->getAllValuesForJoin(left_accessor,
                                                right_accessor,
                                                joined_tuple_ids,
                                                cv_cache));

        return ColumnVectorPtr(
            fast_operator_->applyToValueAccessorAndColumnVectorForJoin(
                left_operand_accessor,
                using_left_relation_for_left_operand,
                left_operand_attr_id,
                *right_result,
                joined_tuple_ids));
      } else if (right_operand_attr_id != -1) {
        const JoinSide join_side = right_operand_->join_side();
        DCHECK(join_side != kNone);
        const bool using_left_relation_for_right_operand = (join_side == kLeftSide);
        ValueAccessor *right_operand_accessor = using_left_relation_for_right_operand ? left_accessor
                                                                                      : right_accessor;

        ColumnVectorPtr left_result(
            left_operand_->getAllValuesForJoin(left_accessor,
                                               right_accessor,
                                               joined_tuple_ids,
                                               cv_cache));
        return ColumnVectorPtr(
            fast_operator_->applyToColumnVectorAndValueAccessorForJoin(
                *left_result,
                right_operand_accessor,
                using_left_relation_for_right_operand,
                right_operand_attr_id,
                joined_tuple_ids));
      }
#endif  // QUICKSTEP_ENABLE_VECTOR_COPY_ELISION_JOIN

      ColumnVectorPtr left_result(
          left_operand_->getAllValuesForJoin(left_accessor,
                                             right_accessor,
                                             joined_tuple_ids,
                                             cv_cache));
      ColumnVectorPtr right_result(
          right_operand_->getAllValuesForJoin(left_accessor,
                                              right_accessor,
                                              joined_tuple_ids,
                                              cv_cache));
      return ColumnVectorPtr(
          fast_operator_->applyToColumnVectors(*left_result, *right_result));
    }
  }
}

void ScalarBinaryExpression::getFieldStringItems(
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

  non_container_child_field_names->emplace_back("left_operand");
  non_container_child_fields->emplace_back(left_operand_.get());
  non_container_child_field_names->emplace_back("right_operand");
  non_container_child_fields->emplace_back(right_operand_.get());
}

}  // namespace quickstep
