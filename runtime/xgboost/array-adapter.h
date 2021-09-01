// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <xgboost/adapter.h>

#include "runtime/kphp_core.h"

namespace vk {
namespace xgboost {

class ArrayAdapterBatch final : public ::xgboost::data::detail::NoMetaInfo {
public:
  class Line {
  public:
    Line(std::size_t row_idx, const array<double> &row) noexcept
      : row_idx_(row_idx)
      , row_(row) {
      assert(row.size().string_size == 0);
    }

    std::size_t Size() const noexcept { return row_.size().int_size; }
    ::xgboost::data::COOTuple GetElement(std::size_t idx) const noexcept;

  private:
    const std::size_t row_idx_{0};
    const array<double> &row_;
    mutable array<double>::const_iterator it_{row_.begin()};
    mutable std::size_t expected_index_{0};
  };

  ArrayAdapterBatch(const array<array<double>> &data, std::size_t num_rows) noexcept
    : data_(data)
    , num_rows_(num_rows) {}

  Line GetLine(std::size_t idx) const noexcept;

  std::size_t Size() const noexcept { return num_rows_; }
  static constexpr bool kIsRowMajor = true;

private:
  const array<array<double>> &data_;
  mutable array<array<double>>::const_iterator it_{data_.begin()};
  mutable std::size_t expected_index_{0};
  const std::size_t num_rows_{0};
};

class ArrayAdapter final : public ::xgboost::data::detail::SingleBatchDataIter<ArrayAdapterBatch> {
public:
  ArrayAdapter(const array<array<double>> &data, std::size_t num_rows, std::size_t num_features) noexcept
    : batch_(data, num_rows)
    , num_rows_(num_rows)
    , num_columns_(num_features) {}

  const ArrayAdapterBatch &Value() const override { return batch_; }
  std::size_t NumRows() const noexcept { return num_rows_; }
  std::size_t NumColumns() const noexcept { return num_columns_; }

private:
  ArrayAdapterBatch batch_;
  std::size_t num_rows_{0};
  std::size_t num_columns_{0};
};

} // namespace xgboost
} // namespace vk
