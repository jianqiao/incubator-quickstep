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

#ifndef QUICKSTEP_QUERY_OPTIMIZER_PHYSICAL_FILTER_INJECTION_HPP_
#define QUICKSTEP_QUERY_OPTIMIZER_PHYSICAL_FILTER_INJECTION_HPP_

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "query_optimizer/expressions/ExpressionUtil.hpp"
#include "query_optimizer/expressions/Predicate.hpp"
#include "query_optimizer/physical/BinaryJoin.hpp"
#include "query_optimizer/physical/Physical.hpp"
#include "query_optimizer/physical/PhysicalType.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {
namespace optimizer {
namespace physical {

/** \addtogroup OptimizerPhysical
 *  @{
 */

class FilterInjection;
typedef std::shared_ptr<const FilterInjection> FilterInjectionPtr;

/**
 * @brief Physical filter injection node.
 */
class FilterInjection : public BinaryJoin {
 public:
  PhysicalType getPhysicalType() const override { return PhysicalType::kFilterInjection; }

  std::string getName() const override {
    if (is_anti_filter_) {
      return "FilterInjection(Anti)";
    } else {
      return "FilterInjection";
    }
  }

  const std::vector<expressions::AttributeReferencePtr>& probe_attributes() const {
    return probe_attributes_;
  }

  const std::vector<expressions::AttributeReferencePtr>& build_attributes() const {
    return build_attributes_;
  }

  const expressions::PredicatePtr& build_side_filter_predicate() const {
    return build_side_filter_predicate_;
  }

  const bool is_anti_filter() const {
    return is_anti_filter_;
  }

  PhysicalPtr copyWithNewChildren(
      const std::vector<PhysicalPtr> &new_children) const override {
    DCHECK_EQ(children().size(), new_children.size());
    return Create(new_children[0],
                  new_children[1],
                  probe_attributes_,
                  build_attributes_,
                  project_expressions(),
                  build_side_filter_predicate_,
                  is_anti_filter_);
  }

  std::vector<expressions::AttributeReferencePtr> getReferencedAttributes() const override;

  bool maybeCopyWithPrunedExpressions(
      const expressions::UnorderedNamedExpressionSet &referenced_expressions,
      PhysicalPtr *output) const override;

  static FilterInjectionPtr Create(
      const PhysicalPtr &probe_child,
      const PhysicalPtr &build_child,
      const std::vector<expressions::AttributeReferencePtr> &probe_attributes,
      const std::vector<expressions::AttributeReferencePtr> &build_attributes,
      const std::vector<expressions::NamedExpressionPtr> &project_expressions,
      const expressions::PredicatePtr &build_side_filter_predicate,
      const bool is_anti_filter) {
    return FilterInjectionPtr(
        new FilterInjection(probe_child,
                            build_child,
                            probe_attributes,
                            build_attributes,
                            project_expressions,
                            build_side_filter_predicate,
                            is_anti_filter));
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
  FilterInjection(
      const PhysicalPtr &probe_child,
      const PhysicalPtr &build_child,
      const std::vector<expressions::AttributeReferencePtr> &probe_attributes,
      const std::vector<expressions::AttributeReferencePtr> &build_attributes,
      const std::vector<expressions::NamedExpressionPtr> &project_expressions,
      const expressions::PredicatePtr &build_side_filter_predicate,
      const bool is_anti_filter)
      : BinaryJoin(probe_child, build_child, project_expressions),
        probe_attributes_(probe_attributes),
        build_attributes_(build_attributes),
        build_side_filter_predicate_(build_side_filter_predicate),
        is_anti_filter_(is_anti_filter) {
  }

  std::vector<expressions::AttributeReferencePtr> probe_attributes_;
  std::vector<expressions::AttributeReferencePtr> build_attributes_;
  expressions::PredicatePtr build_side_filter_predicate_;
  bool is_anti_filter_;

  DISALLOW_COPY_AND_ASSIGN(FilterInjection);
};

/** @} */

}  // namespace physical
}  // namespace optimizer
}  // namespace quickstep

#endif  // QUICKSTEP_QUERY_OPTIMIZER_PHYSICAL_FILTER_INJECTION_HPP_
