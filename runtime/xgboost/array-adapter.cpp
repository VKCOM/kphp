// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/xgboost/array-adapter.h"

namespace vk {
namespace xgboost {

::xgboost::data::COOTuple ArrayAdapterBatch::Line::GetElement(std::size_t idx) const noexcept {
  auto it = it_;
  if (idx == expected_index_) {
    ++expected_index_;
    if (++it_ == row_.end()) {
      it_ = row_.begin();
      expected_index_ = 0;
    }
  } else {
    it = std::next(row_.begin(), idx);
  }

  return {row_idx_, static_cast<size_t>(it.get_int_key()), static_cast<float>(it.get_value())};
}

ArrayAdapterBatch::Line ArrayAdapterBatch::GetLine(std::size_t idx) const noexcept {
  auto it = it_;
  if (idx == expected_index_) {
    ++expected_index_;
    if (++it_ == data_.end()) {
      it_ = data_.begin();
      expected_index_ = 0;
    }
  } else {
    it = std::next(data_.begin(), idx);
  }

  return {idx, it.get_value()};
}

} // namespace xgboost
} // namespace vk
