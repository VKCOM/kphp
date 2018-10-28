#pragma once

#include <unordered_set>

#include "compiler/data/vertex-adaptor.h"

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
  Id():
    ptr(nullptr) {}
  explicit Id(IdData *ptr):
    ptr(ptr) {}
  Id(const Id &id):
    ptr(id.ptr) {}
  Id &operator=(const Id &id) {
    ptr = id.ptr;
    return *this;
  }
  IdData &operator*() const {
    assert (ptr != nullptr);
    return *ptr;
  }
  explicit operator bool() const {
    return ptr != nullptr;
  }
  void clear() {
    delete ptr; //TODO: be very-very carefull with it
    ptr = nullptr;
  }

  IdData *operator->() const {
    assert (ptr != nullptr);
    return ptr;
  }

  bool operator==(const Id<IdData> &other) const {
    return (unsigned long)ptr == (unsigned long)other.ptr;
  }
  bool operator!=(const Id<IdData> &other) const {
    return (unsigned long)ptr != (unsigned long)other.ptr;
  }

  string &str() {
    return ptr->str_val;
  }
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

template<class T>
int get_index(const Id<T> &i) {
  return i->id;
}

template<class T>
void set_index(Id<T> &d, int index) {
  d->id = index;
}

class VarData;
class ClassData;
class DefineData;
class FunctionData;
class SrcFile;

typedef Id<FunctionData> FunctionPtr;
typedef Id<VarData> VarPtr;
typedef Id<ClassData> ClassPtr;
typedef Id<DefineData> DefinePtr;
typedef Id<SrcFile> SrcFilePtr;

bool operator<(FunctionPtr, FunctionPtr);
bool operator<(VarPtr, VarPtr);

