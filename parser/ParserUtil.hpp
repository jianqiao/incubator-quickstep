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

#ifndef QUICKSTEP_PARSER_PARSER_UTIL_HPP_
#define QUICKSTEP_PARSER_PARSER_UTIL_HPP_

#include <vector>

#include "utility/PtrList.hpp"

namespace quickstep {
class ParseStatement;

/**
 * @brief Create a PtrList that takes ownership of a variadic number of
 *        homogeneous objects.
 *
 * @param front The first object.
 * @param rest The remaining objects.
 *
 * @return A PtrList of the input objects.
 */
template <typename T, typename ...Ts>
PtrList<T>* CreatePtrList(T *front, Ts *...rest) {
  PtrList<T> *list = new PtrList<T>();
  list->push_back(front);
  for (T *item : std::vector<T*>{rest...}) {
    list->push_back(item);
  }
  return list;
}

}

struct YYLTYPE;

/** \addtogroup Parser
 *  @{
 */

/**
 * @brief Report a parsing error to STDERR.
 *
 * @param yyloc The error location, which may not be the same as the location in
 *              the token currently being scanned.
 * @param yyscanner The scanner context.
 * @param parsed_statement This parameter exists purely so that yyerror() will
 *        have the type signature expected by the parser, and is ignored.
 * @param s A string describing the error encountered.
 **/
void quickstep_yyerror(const YYLTYPE *yyloc,
                       void *yyscanner,
                       quickstep::ParseStatement **statement,
                       const char *error_message);

/** @} */

#endif  // QUICKSTEP_PARSER_PARSER_UTIL_HPP_
