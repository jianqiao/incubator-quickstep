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

#ifndef QUICKSTEP_QUERY_OPTIMIZER_PHYSICAL_UNION_ALL_HPP_
#define QUICKSTEP_QUERY_OPTIMIZER_PHYSICAL_UNION_ALL_HPP_

#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/physical/Physical.hpp"
#include "query_optimizer/physical/PhysicalType.hpp"
#include "utility/Cast.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

namespace optimizer {
namespace physical {

/** \addtogroup OptimizerPhysical
 *  @{
 */

class UnionAll;
typedef std::shared_ptr<const UnionAll> UnionAllPtr;

/**
 * @brief Perform Union All operation on query results
 */
class UnionAll : public Physical {
 public:

  PhysicalType getPhysicalType() const override {
    return PhysicalType::kUnionAll;
  }

  std::string getName() const override { return "UnionAll"; }

  const std::vector<PhysicalPtr>& operands() const { return operands_; }

  PhysicalPtr copyWithNewChildren(
      const std::vector<PhysicalPtr> &new_children) const override {
    //DCHECK_EQ(getNumChildren(), new_children.size());
    return Create(new_children);
  }

  std::vector<expressions::AttributeReferencePtr> getOutputAttributes() const override {
    return operands_[0]->getOutputAttributes();
  }

  std::vector<expressions::AttributeReferencePtr> getReferencedAttributes() const override {
    std::vector<expressions::AttributeReferencePtr> referenced_attributes;
    for (const PhysicalPtr &operand : operands_) {
      const std::vector<expressions::AttributeReferencePtr> reference =
          operand->getOutputAttributes();
      referenced_attributes.insert(referenced_attributes.end(),
                                   reference.begin(),
                                   reference.end());
    }
    return referenced_attributes;
  }

  bool maybeCopyWithPrunedExpressions(
      const expressions::UnorderedNamedExpressionSet &referenced_expressions,
      PhysicalPtr *output) const override {
    std::vector<PhysicalPtr> new_operands;
    // For UnionAll operation, either all operands are pruned, or non operand is pruned
    // So use operand0 to see if need to be pruned, then use this to check other operands
    PhysicalPtr new_operand0;
    bool return_flag = operands_[0]->maybeCopyWithPrunedExpressions(referenced_expressions, &new_operand0);
    if (return_flag)
      new_operands.push_back(new_operand0);

    // Check the rest of the operands
    for (std::size_t i=1; i<operands_.size(); i++) {
      PhysicalPtr new_operand;
      bool ret = operands_[i]->maybeCopyWithPrunedExpressions(referenced_expressions, &new_operand);
      DCHECK_EQ(return_flag, ret);
      if (ret)
        new_operands.push_back(new_operand);
    }

    if (return_flag)
      *output = Create(new_operands);
    return return_flag;
  }

  /**
   * @brief Creates the physical node of Union All.
   *
   * @param operands The children physical node of union all
   * @return An immutable UnionAll node.
   */
  static UnionAllPtr Create(const std::vector<PhysicalPtr> &operands) {
    return UnionAllPtr(new UnionAll(operands));
  }

 protected:
  void getFieldStringItems(
      std::vector<std::string> *inline_field_names,
      std::vector<std::string> *inline_field_values,
      std::vector<std::string> *non_container_child_field_names,
      std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
      std::vector<std::string> *container_child_field_names,
      std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const override {
    container_child_field_names->push_back("operands");

    container_child_fields->push_back(CastSharedPtrVector<OptimizerTreeBase>(operands_));
  }

 private:
  UnionAll(const std::vector<PhysicalPtr> &operands) {
    for (const PhysicalPtr &operand : operands) {
      operands_.push_back(operand);
      addChild(operand);
    }
  }

  std::vector<PhysicalPtr> operands_;

  DISALLOW_COPY_AND_ASSIGN(UnionAll);
};

/** @} */

}  // namespace physical
}  // namespace optimizer
}  // namespace quickstep

#endif /* QUICKSTEP_QUERY_OPTIMIZER_PHYSICAL_UNION_ALL_HPP_ */
