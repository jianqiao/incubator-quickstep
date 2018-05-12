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

#ifndef QUICKSTEP_TYPES_TYPE_TRAITS_HPP_
#define QUICKSTEP_TYPES_TYPE_TRAITS_HPP_

#include <cstddef>
#include <cstdint>

#include "types/DatetimeLit.hpp"
#include "types/IntervalLit.hpp"
#include "types/NullLit.hpp"
#include "types/Type.hpp"
#include "types/TypeID.hpp"
#include "utility/meta/Common.hpp"
#include "utility/meta/TypeList.hpp"

namespace quickstep {

class TypedValue;

/** \addtogroup Types
 *  @{
 */

using ParInlinePodCppType = const void*;
using ParOutOfLinePodCppType = TypedValue;

/**
 * @brief Traits of Quickstep types.
 */
template <TypeID type_id>
struct TypeTrait;

#define REGISTER_TYPE_TRAIT(type_class, type_id, super_type_id, memory_layout, cpp_type) \
  class type_class;                                                            \
  template <> struct TypeTrait<type_id> {                                      \
    typedef type_class TypeClass;                                              \
    typedef cpp_type cpptype;                                                  \
    static constexpr TypeID kStaticTypeID = type_id;                           \
    static constexpr Type::SuperTypeID kStaticSuperTypeID = super_type_id;     \
    static constexpr MemoryLayout kMemoryLayout = memory_layout;               \
  };

REGISTER_TYPE_TRAIT(IntType, kInt,
                    Type::kNumeric, kCxxInlinePod, int);
REGISTER_TYPE_TRAIT(LongType, kLong,
                    Type::kNumeric, kCxxInlinePod, std::int64_t);
REGISTER_TYPE_TRAIT(FloatType, kFloat,
                    Type::kNumeric, kCxxInlinePod, float);
REGISTER_TYPE_TRAIT(DoubleType, kDouble,
                    Type::kNumeric, kCxxInlinePod, double);
REGISTER_TYPE_TRAIT(DateType, kDate,
                    Type::kOther, kCxxInlinePod, DateLit);
REGISTER_TYPE_TRAIT(DatetimeType, kDatetime,
                    Type::kOther, kCxxInlinePod, DatetimeLit);
REGISTER_TYPE_TRAIT(DatetimeIntervalType, kDatetimeInterval,
                    Type::kOther, kCxxInlinePod, DatetimeIntervalLit);
REGISTER_TYPE_TRAIT(YearMonthIntervalType, kYearMonthInterval,
                    Type::kOther, kCxxInlinePod, YearMonthIntervalLit);
REGISTER_TYPE_TRAIT(CharType, kChar,
                    Type::kAsciiString, kParInlinePod, ParInlinePodCppType);
REGISTER_TYPE_TRAIT(VarCharType, kVarChar,
                    Type::kAsciiString, kParOutOfLinePod, ParOutOfLinePodCppType);
REGISTER_TYPE_TRAIT(NullType, kNullType,
                    Type::kOther, kCxxGeneric, NullLit);

#undef REGISTER_TYPE


// A compile-time sequence of all TypeIDs.
using TypeIDSequenceAll =
    meta::MakeSequence<static_cast<std::size_t>(kNumTypeIDs)>
        ::cast_to<TypeID>;

namespace internal {

// Meta-function for filtering out TypeIDs where the corresponding types have
// the specified SuperTypeID.
template <Type::SuperTypeID super_type_id>
struct SuperTypeIDSelector {
  template <typename Literal>
  struct MetaFunctor {
    static constexpr bool value =
        TypeTrait<Literal::value>::kStaticSuperTypeID == super_type_id;
  };
  using type = typename TypeIDSequenceAll::bind_to<meta::TypeList>
                                         ::filter<MetaFunctor>
                                         ::template as_sequence<TypeID>;
};

// Meta-function for filtering out TypeIDs where the corresponding types have
// the specified MemoryLayout.
template <MemoryLayout memory_layout>
struct MemoryLayoutSelector {
  template <typename Literal>
  struct MetaFunctor {
    static constexpr bool value =
        TypeTrait<Literal::value>::kMemoryLayout == memory_layout;
  };
  using type = typename TypeIDSequenceAll::bind_to<meta::TypeList>
                                         ::filter<MetaFunctor>
                                         ::template as_sequence<TypeID>;
};

}  // namespace internal

// A compile-time sequence of all TypeIDs of Numeric super type.
using TypeIDSequenceNumeric = internal::SuperTypeIDSelector<Type::kNumeric>::type;

// A compile-time sequence of all TypeIDs of AsciiString super type.
using TypeIDSequenceAsciiString = internal::SuperTypeIDSelector<Type::kAsciiString>::type;

// A compile-time sequence of all TypeIDs of CxxInlinePod memory layout.
using TypeIDSequenceCxxInlinePod = internal::MemoryLayoutSelector<kCxxInlinePod>::type;

// A compile-time sequence of all TypeIDs of ParInlinePod memory layout.
using TypeIDSequenceParInlinePod = internal::MemoryLayoutSelector<kParInlinePod>::type;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_TYPE_TRAITS_HPP_
