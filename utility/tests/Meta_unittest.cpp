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

#include <array>
#include <cstddef>
#include <type_traits>

#include "types/DoubleType.hpp"
#include "types/FloatType.hpp"
#include "types/IntType.hpp"
#include "types/LongType.hpp"
#include "types/TypeID.hpp"
#include "utility/meta/Meta.hpp"

#include "gtest/gtest.h"

namespace quickstep {

using meta::TypeList;

template <typename QsType>
struct GetCppTypeSize {
  using type = std::integral_constant<std::size_t, sizeof(typename QsType::cpptype)>;
};

template <typename QsType>
struct SelectTypeOfSizeFour {
  static constexpr bool value = (sizeof(typename QsType::cpptype) == 4);
};

using ZeroValue = std::integral_constant<std::size_t, 0>;

template <typename Lhs, typename Rhs>
struct Sum {
  using type = std::integral_constant<std::size_t, Lhs::value + Rhs::value>;
};

TEST(MetaTest, MakeSequenceTest) {
  using S = meta::MakeSequence<4>;
  EXPECT_TRUE(S::contains<0>::value);
  EXPECT_TRUE(S::contains<1>::value);
  EXPECT_TRUE(S::contains<2>::value);
  EXPECT_TRUE(S::contains<3>::value);

  EXPECT_FALSE(S::contains<4>::value);
  EXPECT_FALSE(S::contains<4096>::value);

  S::bind_to<TypeList>::ForEach([&](auto e) {
    constexpr std::size_t value = decltype(e)::head::value;
    EXPECT_LT(value, 4);
  });
}

TEST(MetaTest, TypeListHeadTailTest) {
  using R = TypeList<IntType, LongType, FloatType, DoubleType>;

  using H = R::head;
  using T = R::tail;

  EXPECT_EQ(H::InstanceNonNullable().getTypeID(), kInt);
  EXPECT_EQ(T::length, 3);

  EXPECT_EQ(T::head::InstanceNonNullable().getTypeID(), kLong);
  EXPECT_EQ(T::tail::head::InstanceNonNullable().getTypeID(), kFloat);
  EXPECT_EQ(T::tail::tail::head::InstanceNonNullable().getTypeID(), kDouble);
}

TEST(MetaTest, TypeListUniqueTest) {
  using R = TypeList<IntType, FloatType, IntType, DoubleType>;
  using U = R::unique<>;

  EXPECT_EQ(U::length, 3);
  EXPECT_EQ(U::at<0>::InstanceNonNullable().getTypeID(), kInt);
  EXPECT_EQ(U::at<1>::InstanceNonNullable().getTypeID(), kFloat);
  EXPECT_EQ(U::at<2>::InstanceNonNullable().getTypeID(), kDouble);
}

TEST(MetaTest, TypeListSkipTakeTest) {
  using R = TypeList<IntType, FloatType, IntType, DoubleType>;
  using U = R::skip<1>::take<2>;

  EXPECT_EQ(U::length, 2);
  EXPECT_EQ(U::at<0>::InstanceNonNullable().getTypeID(), kFloat);
  EXPECT_EQ(U::at<1>::InstanceNonNullable().getTypeID(), kInt);
}

TEST(MetaTest, TypeListApplyTest) {
  using R = TypeList<IntType, LongType, FloatType, DoubleType>;
  using U = R::apply<GetCppTypeSize>;

  EXPECT_EQ(U::length, 4);
  EXPECT_EQ(U::at<0>::value, 4);
  EXPECT_EQ(U::at<1>::value, 8);
  EXPECT_EQ(U::at<2>::value, 4);
  EXPECT_EQ(U::at<3>::value, 8);
}

TEST(MetaTest, TypeListFoldlTest) {
  using R = TypeList<IntType, LongType, FloatType, DoubleType>;
  using U = R::apply<GetCppTypeSize>::foldl<Sum, ZeroValue>;

  EXPECT_EQ(U::value, 24);
}

TEST(MetaTest, TypeListFlattenTest) {
  using R = TypeList<TypeList<TypeList<IntType, LongType>>,
                     TypeList<FloatType, TypeList<DoubleType>>>;
  using U = R::flatten<>;

  EXPECT_EQ(U::length, 4);
  EXPECT_EQ(U::at<0>::InstanceNonNullable().getTypeID(), kInt);
  EXPECT_EQ(U::at<1>::InstanceNonNullable().getTypeID(), kLong);
  EXPECT_EQ(U::at<2>::InstanceNonNullable().getTypeID(), kFloat);
  EXPECT_EQ(U::at<3>::InstanceNonNullable().getTypeID(), kDouble);
}

TEST(MetaTest, TypeListFilterTest) {
  using R = TypeList<IntType, LongType, FloatType, DoubleType>;
  using U = R::filter<SelectTypeOfSizeFour>;

  EXPECT_EQ(U::length, 2);
  EXPECT_EQ(U::at<0>::InstanceNonNullable().getTypeID(), kInt);
  EXPECT_EQ(U::at<1>::InstanceNonNullable().getTypeID(), kFloat);
}

TEST(MetaTest, TypeListZipTest) {
  using R1 = TypeList<IntType, FloatType>;
  using R2 = TypeList<LongType, DoubleType>;
  using U = R1::zip<R2>;

  EXPECT_EQ(U::length, 2);
  EXPECT_EQ((U::at<0, 0>::InstanceNonNullable().getTypeID()), kInt);
  EXPECT_EQ((U::at<0, 1>::InstanceNonNullable().getTypeID()), kLong);
  EXPECT_EQ((U::at<1, 0>::InstanceNonNullable().getTypeID()), kFloat);
  EXPECT_EQ((U::at<1, 1>::InstanceNonNullable().getTypeID()), kDouble);
}

TEST(MetaTest, StringConstantTest) {
  using R = STR_CONST("hello");
  using U = STR_CONST("world");

  DCHECK_EQ(R::ToString(), "hello");
  DCHECK_EQ(U::ToString(), "world");
}

TEST(MetaTest, MultipleDispatcherFirstTest) {
  using Dispatcher = meta::MultipleDispatcher<meta::MakeSequence<4>>;

  const std::size_t value1 = 2;
  const std::size_t value2 = 10;

  EXPECT_TRUE(Dispatcher::IsInvokable(value1));
  EXPECT_FALSE(Dispatcher::IsInvokable(value2));

  Dispatcher::InvokeOn(2, [&](auto typelist) -> void {
    constexpr std::size_t value = decltype(typelist)::head::value;
    EXPECT_EQ(value, 2);
  });
}

TEST(MetaTest, MultipleDispatcherSecondTest) {
  using Dispatcher = meta::MultipleDispatcher<
      meta::Sequence<TypeID, kInt, kLong, kFloat, kDouble>,
      meta::TypeList<IntType, LongType, FloatType, DoubleType>>;

  EXPECT_TRUE(Dispatcher::IsInvokable(kInt));
  EXPECT_TRUE(Dispatcher::IsInvokable(kLong));
  EXPECT_TRUE(Dispatcher::IsInvokable(kFloat));
  EXPECT_TRUE(Dispatcher::IsInvokable(kDouble));

  EXPECT_FALSE(Dispatcher::IsInvokable(kChar));
  EXPECT_FALSE(Dispatcher::IsInvokable(kDate));

  const TypeID tid1 = kInt;
  const TypeID tid2 = kDouble;

  Dispatcher::InvokeOn(tid1, [&](auto typelist) -> void {
    using QsType = typename decltype(typelist)::head;
    EXPECT_EQ(QsType::InstanceNonNullable().getTypeID(), kInt);
  });

  Dispatcher::InvokeOn(tid2, [&](auto typelist) -> void {
    using QsType = typename decltype(typelist)::head;
    EXPECT_EQ(QsType::InstanceNonNullable().getTypeID(), kDouble);
  });
}

TEST(MetaTest, BoolDispatcherTest) {
  using Dispatcher = meta::BoolDispatcher::repeat<3>;

  const std::array<bool, 2> values = {{ true, false }};
  for (const bool v0 : values) {
    for (const bool v1 : values) {
      for (const bool v2 : values) {
        EXPECT_TRUE(Dispatcher::IsInvokable(v0, v1, v2));

        Dispatcher::InvokeOn(
            v0, v1, v2,
            [&](auto typelist) -> void {
          using Params = decltype(typelist);
          constexpr bool a0 = Params::template at<0>::value;
          constexpr bool a1 = Params::template at<1>::value;
          constexpr bool a2 = Params::template at<2>::value;

          EXPECT_EQ(v0, a0);
          EXPECT_EQ(v1, a1);
          EXPECT_EQ(v2, a2);
        });
      }
    }
  }
}

}  // namespace quickstep
