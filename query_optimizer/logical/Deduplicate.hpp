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

#ifndef QUICKSTEP_QUERY_OPTIMIZER_LOGICAL_DEDUPLICATE_HPP_
#define QUICKSTEP_QUERY_OPTIMIZER_LOGICAL_DEDUPLICATE_HPP_

#include <memory>
#include <string>
#include <vector>

#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/expressions/Expression.hpp"
#include "query_optimizer/logical/Logical.hpp"
#include "query_optimizer/logical/LogicalType.hpp"
#include "utility/Macros.hpp"

namespace quickstep {
namespace optimizer {
namespace logical {

/** \addtogroup OptimizerLogical
 *  @{
 */

class Deduplicate;
typedef std::shared_ptr<const Deduplicate> DeduplicatePtr;

/**
 * @brief Deduplicate operator that filters the input relation by eliminating
 *        duplicated tuples.
 */
class Deduplicate : public Logical {
 public:
  LogicalType getLogicalType() const override {
    return LogicalType::kDeduplicate;
  }

  std::string getName() const override {
    return "Deduplicate";
  }

  /**
   * @return The list of attributes to be deduplicated.
   */
  const std::vector<expressions::AttributeReferencePtr>& attribute_list() const {
    return attribute_list_;
  }

  /**
   * @return The input operator.
   */
  const LogicalPtr& input() const {
    return input_;
  }

  LogicalPtr copyWithNewChildren(
      const std::vector<LogicalPtr> &new_children) const override;

  LogicalPtr copyWithNewInputExpressions(
      const std::vector<expressions::ExpressionPtr> &input_expressions) const override;

  std::vector<expressions::AttributeReferencePtr> getOutputAttributes() const override;

  std::vector<expressions::AttributeReferencePtr> getReferencedAttributes() const override;

  /**
   * @brief Creates a Deduplicate that eliminates duplicated tuples.
   *
   * @param input The input operator to this Deduplicate.
   * @param attribute_list The list of attributes to be deduplicated.
   * @return An immutable Deduplicate.
   */
  static DeduplicatePtr Create(
      const LogicalPtr &input,
      const std::vector<expressions::AttributeReferencePtr> &attribute_list) {
    return DeduplicatePtr(new Deduplicate(input, attribute_list));
  }

 protected:
  void getFieldStringItems(
      std::vector<std::string> *inline_field_names,
      std::vector<std::string> *inline_field_values,
      std::vector<std::string> *non_container_child_field_names,
      std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
      std::vector<std::string> *container_child_field_names,
      std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const override;

 private:
  Deduplicate(const LogicalPtr &input,
              const std::vector<expressions::AttributeReferencePtr> &attribute_list)
      : input_(input),
        attribute_list_(attribute_list) {
    addChild(input_);
    addInputExpressions(attribute_list_);
  }

  const LogicalPtr input_;
  const std::vector<expressions::AttributeReferencePtr> attribute_list_;

  DISALLOW_COPY_AND_ASSIGN(Deduplicate);
};

/** @} */

}  // namespace logical
}  // namespace optimizer
}  // namespace quickstep

#endif  // QUICKSTEP_QUERY_OPTIMIZER_LOGICAL_DEDUPLICATE_HPP_
