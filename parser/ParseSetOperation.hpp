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

#ifndef QUICKSTEP_PARSER_PARSE_SET_OPERATION_HPP_
#define QUICKSTEP_PARSER_PARSE_SET_OPERATION_HPP_

#include <string>

#include "parser/ParseTreeNode.hpp"
#include "utility/Macros.hpp"
#include "utility/PtrList.hpp"

namespace quickstep {

/** \addtogroup Parser
 *  @{
 */

/**
 * @brief A parsed representation of Set Operations.
 */
class ParseSetOperation : public ParseTreeNode {
public:
  /**
   * @brief The possible types of set operations
   */
  enum SetOperationType {
    kUnion,
    kUnionAll,
    kIntersect,
    kSingle
  };

  /**
   * @brief Constructor.
   */
  ParseSetOperation(const int line_number,
                    const int column_number,
                    const SetOperationType set_operation_type)
      : ParseTreeNode(line_number, column_number),
        set_operation_type_(set_operation_type) {
    }

  /**
   * @brief Destructor.
   */
  ~ParseSetOperation() override {}

  std::string getName() const override {
    switch (set_operation_type_) {
      case kUnion:
        return "Union";
      case kUnionAll:
        return "UnionAll";
      case kIntersect:
        return "Intersect";
      case kSingle:
        return "Single";
      default:
        return "Unknown";
    }
  }

  const PtrList<ParseTreeNode>& operands() const {
    return operands_;
  }

  void addOperand(ParseTreeNode *operand) {
    operands_.push_back(operand);
  }

  const SetOperationType getOperationType() const {
    return set_operation_type_;
  }

protected:
  void getFieldStringItems(std::vector<std::string> *inline_field_names,
                           std::vector<std::string> *inline_field_values,
                           std::vector<std::string> *non_container_child_field_names,
                           std::vector<const ParseTreeNode*> *non_container_child_fields,
                           std::vector<std::string> *container_child_field_names,
                           std::vector<std::vector<const ParseTreeNode*>> *container_child_fields) const override {
    inline_field_names->push_back("set_operation_type");
    inline_field_values->push_back(getName());

    container_child_field_names->push_back("children");
    container_child_fields->emplace_back();
    for (const ParseTreeNode &child : operands_) {
      container_child_fields->back().push_back(&child);
    }
  }

private:

  PtrList<ParseTreeNode> operands_;

  SetOperationType set_operation_type_;

  DISALLOW_COPY_AND_ASSIGN(ParseSetOperation);
};

/** @} */

}  // namespace quickstep

#endif /* QUICKSTEP_PARSER_PARSE_SET_OPERATION_HPP_ */
