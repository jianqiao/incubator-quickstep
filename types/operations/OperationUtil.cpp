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

#include "types/operations/OperationUtil.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "types/Type.hpp"
#include "types/TypeFactory.hpp"
#include "types/TypeID.hpp"
#include "types/TypedValue.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperationSignatureLite.hpp"

namespace quickstep {

OperationSignaturePtr CreateCast(const Type &source_type,
                                 const Type &target_type) {
  const OperationSignatureLitePtr signature_lite =
      OperationSignatureLite::Create("Cast",
                                     {source_type.getTypeID(), kVarChar},
                                     {false, true});
  const std::string target_type_name = target_type.getName();
  const std::size_t length = target_type_name.length() + 1;
  const Type &target_varchar_type = TypeFactory::GetType(kVarChar, length);

  TypedValue target_varchar_value(kVarChar, target_type_name.c_str(), length);
  target_varchar_value.ensureNotReference();

  return OperationSignature::Create(signature_lite,
                                    {&source_type, &target_varchar_type},
                                    {TypedValue(), std::move(target_varchar_value)});
}

}  // namespace quickstep
