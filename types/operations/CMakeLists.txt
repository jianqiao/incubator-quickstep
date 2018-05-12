# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

add_subdirectory(binary_operations)
add_subdirectory(comparisons)
add_subdirectory(unary_operations)

QS_PROTOBUF_GENERATE_CPP(types_operations_Operation_proto_srcs
                         types_operations_Operation_proto_hdrs
                         Operation.proto)

# Declare micro-libs:
add_library(quickstep_types_operations_Operation Operation.cpp Operation.hpp)
add_library(quickstep_types_operations_OperationFactory OperationFactory.cpp OperationFactory.hpp)
add_library(quickstep_types_operations_OperationSignature OperationSignature.cpp OperationSignature.hpp)
add_library(quickstep_types_operations_OperationSignatureCoercionPrecedence
            OperationSignatureCoercionPrecedence.cpp
            OperationSignatureCoercionPrecedence.hpp)
add_library(quickstep_types_operations_OperationSignatureLite OperationSignatureLite.cpp OperationSignatureLite.hpp)
add_library(quickstep_types_operations_OperationTraits ../../empty_src.cpp OperationTraits.hpp)
add_library(quickstep_types_operations_OperationUtil OperationUtil.cpp OperationUtil.hpp)
add_library(quickstep_types_operations_Operation_proto ${types_operations_Operation_proto_srcs})

# Link dependencies:
target_link_libraries(quickstep_types_operations_Operation
                      quickstep_types_operations_OperationSignature
                      quickstep_types_operations_OperationSignatureLite
                      quickstep_utility_Macros)
target_link_libraries(quickstep_types_operations_OperationFactory
                      glog
                      quickstep_types_TypeID
                      quickstep_types_operations_Operation
                      quickstep_types_operations_OperationSignature
                      quickstep_types_operations_OperationSignatureCoercionPrecedence
                      quickstep_types_operations_OperationSignatureLite
                      quickstep_types_operations_binaryoperations_BinaryOperation
                      quickstep_types_operations_unaryoperations_ArithmeticUnaryOperations
                      quickstep_types_operations_unaryoperations_CastOperation
                      quickstep_types_operations_unaryoperations_UnaryOperation
                      quickstep_types_operations_unaryoperations_UnaryOperationCodeGen
                      quickstep_utility_HashPair
                      quickstep_utility_Macros)
target_link_libraries(quickstep_types_operations_OperationSignature
                      glog
                      quickstep_types_CastUtil
                      quickstep_types_Type
                      quickstep_types_TypeFactory
                      quickstep_types_TypedValue
                      quickstep_types_operations_OperationSignatureLite
                      quickstep_types_operations_Operation_proto
                      quickstep_utility_HashPair
                      quickstep_utility_Macros)
target_link_libraries(quickstep_types_operations_OperationSignatureCoercionPrecedence
                      glog
                      quickstep_utility_Macros)
target_link_libraries(quickstep_types_operations_OperationSignatureLite
                      glog
                      quickstep_types_CastUtil
                      quickstep_types_TypeID
                      quickstep_types_operations_OperationSignatureCoercionPrecedence
                      quickstep_types_operations_Operation_proto
                      quickstep_utility_HashPair
                      quickstep_utility_Macros
                      quickstep_utility_StringUtil)
target_link_libraries(quickstep_types_operations_OperationTraits
                      quickstep_catalog_CatalogTypedefs
                      quickstep_types_Type
                      quickstep_types_TypedValue
                      quickstep_types_containers_ColumnVector
                      quickstep_utility_Macros
                      quickstep_utility_meta_TypeList)
target_link_libraries(quickstep_types_operations_OperationUtil
                      quickstep_types_Type
                      quickstep_types_TypeFactory
                      quickstep_types_TypeID
                      quickstep_types_TypedValue
                      quickstep_types_operations_OperationSignature
                      quickstep_types_operations_OperationSignatureLite)
target_link_libraries(quickstep_types_operations_Operation_proto
                      ${PROTOBUF_LIBRARY}
                      quickstep_types_Type_proto
                      quickstep_types_TypedValue_proto)

# Module all-in-one library:
add_library(quickstep_types_operations ../../empty_src.cpp)
target_link_libraries(quickstep_types_operations
                      quickstep_types_operations_Operation
                      quickstep_types_operations_OperationFactory
                      quickstep_types_operations_OperationSignature
                      quickstep_types_operations_OperationSignatureCoercionPrecedence
                      quickstep_types_operations_OperationSignatureLite
                      quickstep_types_operations_OperationTraits
                      quickstep_types_operations_OperationUtil
                      quickstep_types_operations_Operation_proto
                      quickstep_types_operations_binaryoperations
                      quickstep_types_operations_comparisons
                      quickstep_types_operations_unaryoperations)

# Tests:
add_executable(OperationSignature_tests
               "${CMAKE_CURRENT_SOURCE_DIR}/tests/OperationSignature_unittest.cpp")
target_link_libraries(OperationSignature_tests
                      gtest
                      gtest_main
                      quickstep_types_CharType
                      quickstep_types_FloatType
                      quickstep_types_IntType
                      quickstep_types_LongType
                      quickstep_types_TypeID
                      quickstep_types_operations_OperationSignature
                      quickstep_types_operations_OperationSignatureLite)
add_test(OperationSignature_tests OperationSignature_tests)
