// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/stage.h"

template<class DataType>
struct IdMap {
  vector <DataType> data;

  using iterator = typename vector<DataType>::iterator;
  IdMap() = default;
  explicit IdMap(int size);

  template<class IndexType>
  DataType &operator[](const IndexType &i);
  template<class IndexType>
  const DataType &operator[](const IndexType &i) const;

  iterator begin();
  iterator end();
  void clear();

  void update_size(int n);
};

template<class DataType>
IdMap<DataType>::IdMap(int size) :
  data(size) {
}

template<class DataType>
template<class IndexType>
DataType &IdMap<DataType>::operator[](const IndexType &i) {
  int index = get_index(i);
  kphp_assert_msg(index >= 0, "maybe you've forgotten pass function to stream");
  kphp_assert_msg(index < (int)data.size(), fmt_format("{} of {}\n", index, (int)data.size()));
  return data[index];
}

template<class DataType>
template<class IndexType>
const DataType &IdMap<DataType>::operator[](const IndexType &i) const {
  int index = get_index(i);
  kphp_assert(index >= 0);
  kphp_assert_msg(index < (int)data.size(), fmt_format("{} of {}\n", index, (int)data.size()));
  return data[index];
}

template<class DataType>
typename IdMap<DataType>::iterator IdMap<DataType>::begin() {
  return data.begin();
}

template<class DataType>
typename IdMap<DataType>::iterator IdMap<DataType>::end() {
  return data.end();
}

template<class DataType>
void IdMap<DataType>::clear() {
  data.clear();
}

template<class DataType>
void IdMap<DataType>::update_size(int n) {
  kphp_assert((int)data.size() <= n);
  data.resize(n);
}
