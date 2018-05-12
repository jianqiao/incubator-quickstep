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

#include <string>

#include "types/CharType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/TypeID.hpp"
#include "types/operations/OperationSignature.hpp"
#include "types/operations/OperationSignatureLite.hpp"

#include "gtest/gtest.h"

namespace quickstep {

TEST(OperationSignatureLiteTest, CreateTest) {
  // "+" operation
  const auto op_sig_1 = OperationSignatureLite::Create("+", {kInt, kFloat});
  EXPECT_EQ(op_sig_1->getOperationName(), "+");
  EXPECT_EQ(op_sig_1->getNumParameters(), 2u);
  EXPECT_EQ(op_sig_1->getParameterTypeID(0), kInt);
  EXPECT_EQ(op_sig_1->getParameterTypeID(1), kFloat);
  EXPECT_FALSE(op_sig_1->isStaticParameter(0));
  EXPECT_FALSE(op_sig_1->isStaticParameter(1));
  EXPECT_EQ(op_sig_1->toString(), "+(Int, Float)");

  // SUBSTRING operation
  const auto op_sig_2 = OperationSignatureLite::Create("Substring",
                                                       {kChar, kLong, kLong},
                                                       {false, true, true});
  EXPECT_EQ(op_sig_2->getOperationName(), "Substring");
  EXPECT_EQ(op_sig_2->getNumParameters(), 3u);
  EXPECT_EQ(op_sig_2->getParameterTypeID(0), kChar);
  EXPECT_EQ(op_sig_2->getParameterTypeID(1), kLong);
  EXPECT_EQ(op_sig_2->getParameterTypeID(2), kLong);
  EXPECT_FALSE(op_sig_2->isStaticParameter(0));
  EXPECT_TRUE(op_sig_2->isStaticParameter(1));
  EXPECT_TRUE(op_sig_2->isStaticParameter(2));
  EXPECT_EQ(op_sig_2->toString(), "Substring(Char, static(Long), static(Long))");
}

TEST(OperationSignatureLiteTest, CompareTest) {
  const auto op_sig_1 = OperationSignatureLite::Create("+", {kInt, kInt});
  const auto op_sig_2 = OperationSignatureLite::Create("+", {kInt, kFloat});
  const auto op_sig_3 = OperationSignatureLite::Create("-", {kInt, kFloat});
  const auto op_sig_4 = OperationSignatureLite::Create("+",
                                                       {kInt, kFloat},
                                                       {false, false});
  const auto op_sig_5 = OperationSignatureLite::Create("+",
                                                       {kInt, kFloat},
                                                       {false, true});

  EXPECT_NE(*op_sig_1, *op_sig_2);
  EXPECT_NE(*op_sig_2, *op_sig_3);
  EXPECT_EQ(*op_sig_2, *op_sig_4);
  EXPECT_EQ(op_sig_2->hash(), op_sig_4->hash());
  EXPECT_NE(*op_sig_2, *op_sig_5);
  EXPECT_NE(*op_sig_4, *op_sig_5);
}

TEST(OperationSignatureTest, CreateTest) {
  const Type *char_type = &CharType::InstanceNonNullable(32);
  const Type *float_type = &FloatType::InstanceNonNullable();
  const Type *int_type = &IntType::InstanceNonNullable();
  const Type *long_type = &LongType::InstanceNonNullable();

  // "+" operation
  const auto op_sig_lite_1 =
      OperationSignatureLite::Create("+", {kInt, kFloat});
  const auto op_sig_1 =
      OperationSignature::Create(op_sig_lite_1, {int_type, float_type});
  EXPECT_EQ(op_sig_1->getOperationName(), "+");
  EXPECT_EQ(op_sig_1->getNumParameters(), 2u);
  EXPECT_EQ(&op_sig_1->getParameterType(0), int_type);
  EXPECT_EQ(&op_sig_1->getParameterType(1), float_type);
  EXPECT_FALSE(op_sig_1->isStaticParameter(0));
  EXPECT_FALSE(op_sig_1->isStaticParameter(1));
  EXPECT_EQ(op_sig_1->toString(), "+(Int, Float)");

  // SUBSTRING operation
  const auto op_sig_lite_2 =
      OperationSignatureLite::Create("Substring",
                                     {kChar, kLong, kLong},
                                     {false, true, true});
  const auto op_sig_2 =
      OperationSignature::Create(op_sig_lite_2,
                                 {char_type, long_type, long_type},
                                 {TypedValue(), TypedValue(1ll), TypedValue(10ll)});
  EXPECT_EQ(op_sig_2->getOperationName(), "Substring");
  EXPECT_EQ(op_sig_2->getNumParameters(), 3u);
  EXPECT_EQ(&op_sig_2->getParameterType(0), char_type);
  EXPECT_EQ(&op_sig_2->getParameterType(1), long_type);
  EXPECT_EQ(&op_sig_2->getParameterType(2), long_type);
  EXPECT_FALSE(op_sig_2->isStaticParameter(0));
  EXPECT_TRUE(op_sig_2->isStaticParameter(1));
  EXPECT_TRUE(op_sig_2->isStaticParameter(2));
  EXPECT_EQ(op_sig_2->toString(), "Substring(Char(32), 1, 10)");
}

TEST(OperationSignatureTest, CompareTest) {
  const Type *float_type = &FloatType::InstanceNonNullable();
  const Type *int_type = &IntType::InstanceNonNullable();

  const auto op_sig_lite_1 = OperationSignatureLite::Create("+", {kInt, kInt});
  const auto op_sig_lite_2 = OperationSignatureLite::Create("+", {kInt, kFloat});
  const auto op_sig_lite_3 = OperationSignatureLite::Create("-", {kInt, kFloat});
  const auto op_sig_lite_4 = OperationSignatureLite::Create("+",
                                                            {kInt, kFloat},
                                                            {false, false});
  const auto op_sig_lite_5 = OperationSignatureLite::Create("+",
                                                            {kInt, kFloat},
                                                            {false, true});

  const auto op_sig_1 = OperationSignature::Create(op_sig_lite_1,
                                                   {int_type, int_type});
  const auto op_sig_2 = OperationSignature::Create(op_sig_lite_2,
                                                   {int_type, float_type});
  const auto op_sig_3 = OperationSignature::Create(op_sig_lite_3,
                                                   {int_type, float_type});
  const auto op_sig_4 = OperationSignature::Create(op_sig_lite_4,
                                                   {int_type, float_type});
  const auto op_sig_5 = OperationSignature::Create(op_sig_lite_5,
                                                   {int_type, float_type},
                                                   {TypedValue(), TypedValue(1.0f)});

  EXPECT_NE(*op_sig_1, *op_sig_2);
  EXPECT_NE(*op_sig_2, *op_sig_3);
  EXPECT_EQ(*op_sig_2, *op_sig_4);
  EXPECT_EQ(op_sig_2->hash(), op_sig_4->hash());
  EXPECT_NE(*op_sig_2, *op_sig_5);
  EXPECT_NE(*op_sig_4, *op_sig_5);
}

}  // namespace quickstep
