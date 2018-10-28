#include "compiler/utils.h"

/*** Id ***/
template<class IdData>
Id<IdData>::Id() :
  ptr(nullptr) {}

template<class IdData>
Id<IdData>::Id(IdData *ptr) :
  ptr(ptr) {}

template<class IdData>
Id<IdData>::Id(const Id<IdData> &id) :
  ptr(id.ptr) {}

template<class IdData>
Id<IdData> &Id<IdData>::operator=(const Id<IdData> &id) {
  ptr = id.ptr;
  return *this;
}

template<class IdData>
IdData &Id<IdData>::operator*() const {
  assert (ptr != nullptr);
  return *ptr;
}

template<class IdData>
IdData *Id<IdData>::operator->() const {
  assert (ptr != nullptr);
  return ptr;
}

template<class IdData>
inline string &Id<IdData>::str() {
  return ptr->str_val;
}

template<class IdData>
void Id<IdData>::clear() {
  delete ptr; //TODO: be very-very carefull with it
  ptr = nullptr;
}

/*** [get|set]_index ***/
template<class T>
int get_index(const Id<T> &i) {
  return i->id;
}

template<class T>
void set_index(Id<T> &d, int index) {
  d->id = index;
}

/*** IdMap ***/
template<class DataType>
IdMap<DataType>::IdMap(int size) :
  data(size) {
}

template<class DataType>
template<class IndexType>
DataType &IdMap<DataType>::operator[](const IndexType &i) {
  int index = get_index(i);
  assert (index >= 0);
  dl_assert (index < (int)data.size(), dl_pstr("%d of %d\n", index, (int)data.size()));
  return data[index];
}

template<class DataType>
template<class IndexType>
const DataType &IdMap<DataType>::operator[](const IndexType &i) const {
  int index = get_index(i);
  assert (index >= 0);
  dl_assert (index < (int)data.size(), dl_pstr("%d of %d\n", index, (int)data.size()));
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
  assert ((int)data.size() <= n);
  data.resize(n);
}

/*** IdGen ***/
template<class IdType>
IdGen<IdType>::IdGen() :
  n(0) {
}

template<class IdType>
void IdGen<IdType>::add_id_map(IdMapBase *to_add) {
  to_add->update_size((n | 0xff) + 1);
  id_maps.push_back(to_add);
}

template<class IdType>
int IdGen<IdType>::init_id(IdType *to_add) {
  set_index(*to_add, n);

  if ((n & 0xff) == 0) {
    int real_n = (n | 0xff) + 1;
    ids.update_size(real_n);

    for (int i = 0; i < (int)id_maps.size(); i++) {
      id_maps[i]->update_size(real_n);
    }
  }

  ids[*to_add] = *to_add;
  n++;
  return n - 1;
}

template<class IdType>
int IdGen<IdType>::size() {
  return n;
}

template<class IdType>
typename IdGen<IdType>::iterator IdGen<IdType>::begin() {
  return ids.begin();
}

template<class IdType>
typename IdGen<IdType>::iterator IdGen<IdType>::end() {
  return ids.begin() + size();
}

template<class IdType>
void IdGen<IdType>::clear() {
  n = 0;
  for (auto &x: ids) {
    x.clear();
  }
  ids.clear();
  for (int i = 0; i < (int)id_maps.size(); i++) {
    id_maps[i]->clear();
  }
  id_maps.clear();
}
