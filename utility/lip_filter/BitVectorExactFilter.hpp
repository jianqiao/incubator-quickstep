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

#ifndef QUICKSTEP_UTILITY_LIP_FILTER_BIT_VECTOR_EXACT_FILTER_HPP_
#define QUICKSTEP_UTILITY_LIP_FILTER_BIT_VECTOR_EXACT_FILTER_HPP_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "catalog/CatalogTypedefs.hpp"
#include "storage/StorageBlockInfo.hpp"
#include "storage/StorageConstants.hpp"
#include "storage/ValueAccessor.hpp"
#include "storage/ValueAccessorUtil.hpp"
#include "types/Type.hpp"
#include "utility/Macros.hpp"
#include "utility/lip_filter/LIPFilter.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Utility
 *  @{
 */

template <typename CppType, bool is_anti_filter>
class BitVectorExactFilter : public LIPFilter {
 public:
  /**
   * @brief Constructor.
   *
   * @param filter_cardinality The cardinality of this hash filter.
   */
  explicit BitVectorExactFilter(const std::size_t filter_cardinality)
      : LIPFilter(LIPFilterType::kBitVectorExactFilter),
        filter_cardinality_(filter_cardinality),
        bit_array_(GetByteSize(filter_cardinality)) {
    DCHECK_GE(filter_cardinality, 0u);
    std::memset(bit_array_.data(),
                0x0,
                sizeof(std::atomic<std::uint8_t>) * GetByteSize(filter_cardinality));
  }

  void insertValueAccessor(ValueAccessor *accessor,
                           const attribute_id attr_id,
                           const Type *attr_type) override {
    InvokeOnAnyValueAccessor(
        accessor,
        [&](auto *accessor) -> void {  // NOLINT(build/c++11)
      if (attr_type->isNullable()) {
        this->insertValueAccessorInternal<true>(accessor, attr_id);
      } else {
        this->insertValueAccessorInternal<false>(accessor, attr_id);
      }
    });
  }

  std::size_t filterBatch(ValueAccessor *accessor,
                          const attribute_id attr_id,
                          const bool is_attr_nullable,
                          std::vector<tuple_id> *batch,
                          const std::size_t batch_size) const override {
    DCHECK(batch != nullptr);
    DCHECK_LE(batch_size, batch->size());

    return InvokeOnAnyValueAccessor(
        accessor,
        [&](auto *accessor) -> std::size_t {  // NOLINT(build/c++11)
      if (is_attr_nullable) {
        return this->filterBatchInternal<true>(accessor, attr_id, batch, batch_size);
      } else {
        return this->filterBatchInternal<false>(accessor, attr_id, batch, batch_size);
      }
    });
  }

 private:
  /**
   * @brief Round up bit_size to multiples of 8.
   */
  inline static std::size_t GetByteSize(const std::size_t bit_size) {
    return (bit_size + 7) / 8;
  }

  /**
   * @brief Iterate through the accessor and hash values into the internal bit
   *        array.
   */
  template <bool is_attr_nullable, typename ValueAccessorT>
  inline void insertValueAccessorInternal(ValueAccessorT *accessor,
                                          const attribute_id attr_id) {
    accessor->beginIteration();
    while (accessor->next()) {
      const void *value = accessor->template getUntypedValue<is_attr_nullable>(attr_id);
      if (!is_attr_nullable || value != nullptr) {
        insert(value);
      }
    }
  }

  /**
   * @brief Filter the given batch of tuples from a ValueAccessor. Write the
   *        tuple ids which survive in the filtering back to \p batch.
   */
  template <bool is_attr_nullable, typename ValueAccessorT>
  inline std::size_t filterBatchInternal(const ValueAccessorT *accessor,
                                         const attribute_id attr_id,
                                         std::vector<tuple_id> *batch,
                                         const std::size_t batch_size) const {
    std::size_t out_size = 0;
    for (std::size_t i = 0; i < batch_size; ++i) {
      const tuple_id tid = batch->at(i);
      const void *value =
          accessor->template getUntypedValueAtAbsolutePosition(attr_id, tid);
      if (is_attr_nullable && value == nullptr) {
        continue;
      }
      if (contains(value)) {
        batch->at(out_size) = tid;
        ++out_size;
      }
    }
    return out_size;
  }

  /**
   * @brief Inserts a given value into the exact filter.
   */
  inline void insert(const void *key_begin) {
    const CppType loc = *reinterpret_cast<const CppType *>(key_begin);
    DCHECK_LE(loc, filter_cardinality_);
    bit_array_[loc >> 3u].fetch_or(1u << (loc & 7u), std::memory_order_relaxed);
  }

  /**
   * @brief Test membership of a given value in the exact filter.
   */
  inline bool contains(const void *key_begin) const {
    const CppType loc = *reinterpret_cast<const CppType *>(key_begin);
    DCHECK_LE(loc, filter_cardinality_);
    const bool is_bit_set =
        (bit_array_[loc >> 3u].load(std::memory_order_relaxed) & (1u << (loc & 7u))) != 0;
    if (is_anti_filter) {
      return !is_bit_set;
    } else {
      return is_bit_set;
    }
  }

  std::size_t filter_cardinality_;
  alignas(kCacheLineBytes) std::vector<std::atomic<std::uint8_t>> bit_array_;

  DISALLOW_COPY_AND_ASSIGN(BitVectorExactFilter);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_LIP_FILTER_BIT_VECTOR_EXACT_FILTER_HPP_
