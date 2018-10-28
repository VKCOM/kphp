#pragma once

#include <unordered_set>

#include "compiler/common.h"
#include "common/wrappers/iterator_range.h"

/*** Id ***/
template<class IdData>
class Id {
  IdData *ptr;
public:
  struct Hash {
    size_t operator()(const Id<IdData> &arg) const noexcept {
      return reinterpret_cast<size_t>(arg.ptr);
    }
  };
public:
  Id();
  explicit Id(IdData *ptr);
  Id(const Id &id);
  Id &operator=(const Id &id);
  IdData &operator*() const;
  explicit operator bool() const {
    return ptr != nullptr;
  }
  void clear();

  IdData *operator->() const;

  bool operator==(const Id<IdData> &other) const {
    return (unsigned long)ptr == (unsigned long)other.ptr;
  }
  bool operator!=(const Id<IdData> &other) const {
    return (unsigned long)ptr != (unsigned long)other.ptr;
  }

  string &str();
};

template<class T>
void my_unique(std::vector<Id<T>> *v) {
  if (v->empty()) {
    return;
  }

  std::unordered_set<Id<T>, typename Id<T>::Hash> us(v->begin(), v->end());
  v->assign(us.begin(), us.end());
  std::sort(v->begin(), v->end());
}

/*** [get|set]_index ***/
template<class T>
int get_index(const Id<T> &i);

template<class T>
void set_index(Id<T> &d, int index);

/*** IdMapBase ***/
struct IdMapBase {
  virtual void update_size(int new_max_id) = 0;
  virtual void clear() = 0;

  virtual ~IdMapBase() {}
};

/*** IdMap ***/
template<class DataType>
struct IdMap : public IdMapBase {
  vector<DataType> data;

  typedef typename vector<DataType>::iterator iterator;
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

/*** IdGen ***/
template<class IdType>
struct IdGen {
  typedef typename IdMap<IdType>::iterator iterator;

  vector<IdMapBase *> id_maps;
  IdMap<IdType> ids;
  int n;

  IdGen();
  void add_id_map(IdMapBase *to_add);

  int init_id(IdType *to_add);
  int size();
  iterator begin();
  iterator end();
  void clear();
};

#include "graph.hpp"
