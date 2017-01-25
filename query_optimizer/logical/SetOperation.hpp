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

#ifndef QUICKSTEP_QUERY_OPTIMIZER_LOGICAL_SETOPERATION_HPP_
#define QUICKSTEP_QUERY_OPTIMIZER_LOGICAL_SETOPERATION_HPP_

#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/logical/Logical.hpp"
#include "query_optimizer/logical/LogicalType.hpp"
#include "utility/Cast.hpp"
#include "utility/Macros.hpp"

namespace quickstep {
namespace optimizer {
namespace logical {

/** \addtogroup OptimizerLogical
 *  @{
 */

class SetOperation;
typedef std::shared_ptr<const SetOperation> SetOperationPtr;

/**
 * @brief Base class for logical set operation operator that combines tuples from two or
 *        more relations.
 */
class SetOperation : public Logical {
 public:
   enum SetOperationType {
    kUnion,
    kUnionAll,
    kIntersect
   };

  /**
   * @brief Destructor.
   */
  ~SetOperation() override {}

  LogicalType getLogicalType() const override {
    return LogicalType::kSetOperation;
  }

  std::string getName() const override {
    switch (set_operation_type_) {
      case SetOperationType::kUnion:
        return "Union";
      case SetOperationType::kUnionAll:
        return "UnionAll";
      case SetOperationType::kIntersect:
        return "Intersect";
      default:
        LOG(FATAL) << "Invalid set operation type: "
                   << static_cast<typename std::underlying_type<SetOperationType>::type>(set_operation_type_);
    }
  }

  // Set operator requires all operands have the same output attribute (maybe by cast)
  // So the output attribute of first operand is the output attribute of all the operands
  std::vector<expressions::AttributeReferencePtr> getOutputAttributes() const override {
    return operands_.front()->getOutputAttributes();
  }

  std::vector<expressions::AttributeReferencePtr> getReferencedAttributes() const override {
    std::vector<expressions::AttributeReferencePtr> referenced_attributes;
    for (const auto &operand : operands_) {
      const std::vector<expressions::AttributeReferencePtr> reference =
          operand->getOutputAttributes();
      referenced_attributes.insert(referenced_attributes.end(),
                                   reference.begin(),
                                   reference.end());
    }
    return referenced_attributes;
  }

  LogicalPtr copyWithNewChildren(
      const std::vector<LogicalPtr> &new_children) const override {
    //DCHECK_EQ(new_children.size(), children().size());
    return SetOperation::Create(new_children, set_operation_type_);
  }

  SetOperationType getSetOperationType() const {
    return set_operation_type_;
  }

  const std::vector<LogicalPtr>& getOperands() const {
    return operands_;
  }

  static SetOperationPtr Create(const std::vector<LogicalPtr> &operands, const SetOperationType type) {
    DCHECK_GT(operands.size(), 1u);
    return SetOperationPtr(new SetOperation(operands, type));
  }

 protected:

  void getFieldStringItems(
      std::vector<std::string> *inline_field_names,
      std::vector<std::string> *inline_field_values,
      std::vector<std::string> *non_container_child_field_names,
      std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
      std::vector<std::string> *container_child_field_names,
      std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const override {
    inline_field_names->push_back("set_operation_type");
    inline_field_values->push_back(getName());

    container_child_field_names->push_back("operands");
    container_child_fields->push_back(
          CastSharedPtrVector<OptimizerTreeBase>(operands_));
  }

 private:

  /**
   * @brief Constructor.
   */
  SetOperation(const std::vector<LogicalPtr> &operands, const SetOperationType set_operation_type)
    : set_operation_type_(set_operation_type) {
      for (const LogicalPtr& operand : operands) {
        operands_.push_back(operand);
        addChild(operand);
      }
  }

  SetOperationType set_operation_type_;

  std::vector<LogicalPtr> operands_;

  DISALLOW_COPY_AND_ASSIGN(SetOperation);
};

/** @} */

}  // namespace logical
}  // namespace optimizer
}  // namespace quickstep

#endif /* QUICKSTEP_QUERY_OPTIMIZER_LOGICAL_SETOPERATION_HPP_ */
