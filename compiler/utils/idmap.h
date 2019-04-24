#pragma once

struct IdMapBase {
  virtual void update_size(int new_max_id) = 0;
  virtual void clear() = 0;

  virtual ~IdMapBase() {}
};

template<class DataType>
struct IdMap : public IdMapBase {
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
  assert(index >= 0 && "maybe you've forgotten pass function to stream");
  dl_assert(index < (int)data.size(), format("%d of %d\n", index, (int)data.size()));
  return data[index];
}

template<class DataType>
template<class IndexType>
const DataType &IdMap<DataType>::operator[](const IndexType &i) const {
  int index = get_index(i);
  assert(index >= 0);
  dl_assert(index < (int)data.size(), format("%d of %d\n", index, (int)data.size()));
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
  assert((int)data.size() <= n);
  data.resize(n);
}
