// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "compiler/kphp_assert.h"

template<class IdData>
class Id {
  IdData* ptr = nullptr;

public:
  struct Hash {
    size_t operator()(const Id<IdData>& arg) const noexcept {
      return reinterpret_cast<size_t>(arg.ptr);
    }
  };

public:
  Id() = default;

  explicit Id(IdData* ptr)
      : ptr(ptr) {}

  Id(const Id& id) = default;

  Id(Id&& id) noexcept
      : ptr(id.ptr) {
    id.ptr = nullptr;
  }

  template<class DerivedIdData>
  Id(const Id<DerivedIdData>& derived)
      : ptr(derived.ptr) {}

  Id& operator=(Id id) noexcept {
    std::swap(ptr, id.ptr);
    return *this;
  }

  template<class DerivedIdData>
  Id& operator=(const Id<DerivedIdData>& derived) {
    ptr = derived.ptr;
    return *this;
  }

  IdData& operator*() const {
    kphp_assert(ptr != nullptr);
    return *ptr;
  }

  explicit operator bool() const {
    return ptr != nullptr;
  }

  IdData* operator->() const {
    kphp_assert(ptr != nullptr);
    return ptr;
  }

  bool operator==(const Id<IdData>& other) const {
    return ptr == other.ptr;
  }

  bool operator!=(const Id<IdData>& other) const {
    return !(*this == other);
  }
};

template<class T>
void my_unique(std::vector<Id<T>>* v) {
  if (v->empty()) {
    return;
  }

  std::unordered_set<Id<T>, typename Id<T>::Hash> us(v->begin(), v->end());
  v->assign(us.begin(), us.end());
  std::sort(v->begin(), v->end());
}

template<class T>
int get_index(const Id<T>& i) {
  return i->id;
}

template<class T>
void set_index(Id<T>& d, int index) {
  d->id = index;
}

class VarData;
class ClassData;
class DefineData;
class FunctionData;
class LibData;
class SrcDir;
class SrcFile;
class ModuliteData;
class ComposerJsonData;
class TypeHint;
class PhpDocComment;
class GenericsInstantiationMixin;
class GenericsInstantiationPhpComment;

using VarPtr = Id<VarData>;
using ClassPtr = Id<ClassData>;
using InterfacePtr = Id<ClassData>;
using TraitPtr = Id<ClassData>;
using DefinePtr = Id<DefineData>;
using FunctionPtr = Id<FunctionData>;
using LibPtr = Id<LibData>;
using SrcDirPtr = Id<SrcDir>;
using SrcFilePtr = Id<SrcFile>;
using ModulitePtr = Id<ModuliteData>;
using ComposerJsonPtr = Id<ComposerJsonData>;

bool operator<(FunctionPtr, FunctionPtr);
bool operator<(VarPtr, VarPtr);

namespace std {
template<typename Data>
struct hash<Id<Data>> : private Id<Data>::Hash {
  using argument_type = Id<Data>;
  using result_type = std::size_t;
  using Id<Data>::Hash::operator();
};
} // namespace std
