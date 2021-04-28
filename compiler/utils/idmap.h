// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/stage.h"
#include "common/wrappers/likely.h"

template<class DataType>
class IdMap {
  std::vector<DataType> data;

  void print_error_invalid_index(int index) const __attribute__((noinline));

public:
  using iterator = typename std::vector<DataType>::iterator;
  
  IdMap() = default;
  explicit IdMap(int size): data(size) {}

  template<class IndexType>
  inline DataType &operator[](const IndexType &i);
  template<class IndexType>
  inline const DataType &operator[](const IndexType &i) const;

  iterator begin() { return data.begin(); }
  iterator end() { return data.end(); }
  void clear() { data.clear(); }

  void update_size(int n);
};

template<class DataType>
template<class IndexType>
DataType &IdMap<DataType>::operator[](const IndexType &i) {
  int index = get_index(i);
  if (unlikely(index < 0 || index >= data.size())) {
    print_error_invalid_index(index);
  }
  return data[index];
}

template<class DataType>
template<class IndexType>
const DataType &IdMap<DataType>::operator[](const IndexType &i) const {
  int index = get_index(i);
  if (unlikely(index < 0 || index >= data.size())) {
    print_error_invalid_index(index);
  }
  return data[index];
}

template<class DataType>
void IdMap<DataType>::print_error_invalid_index(int index) const {
  kphp_assert_msg(index >= 0, "invalid index of IdMap: -1 (maybe you've forgotten to pass a function to stream?)");
  kphp_assert_msg(index < data.size(), fmt_format("invalid index of IdMap: {} (size={})\n", index, data.size()));
}

template<class DataType>
void IdMap<DataType>::update_size(int n) {
  kphp_assert((int)data.size() <= n);
  data.resize(n);
}
