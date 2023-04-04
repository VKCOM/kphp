// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <utility>

#include "common/php-functions.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/vertex.h"
#include "compiler/data/var-data.h"


struct DepLevelContainer { // TODO(mkornaukhov03): in case of empty container begin() != end()
    DepLevelContainer() : mapping(), cnt(0) {
      mapping.reserve(DEFAULT_MAX_DEP_LEVEL);
    }

    struct const_iterator {
        const_iterator(const DepLevelContainer &owner_, size_t dep_level_, size_t internal_index_)
          : owner(owner_), dep_level(dep_level_), internal_index(internal_index_) {}

        const VarPtr &operator*() {
          return owner.mapping[dep_level][internal_index];
        }

        const_iterator &operator++() {
          ++internal_index;
          while (dep_level < owner.mapping.size() && internal_index >= owner.mapping[dep_level].size()) {
            internal_index = 0;
            ++dep_level;
          }
          if (dep_level == owner.mapping.size()) {
            internal_index = 0;
          }
          return *this;
        }

        bool operator==(const const_iterator &oth) const {
          return std::make_tuple(&owner, dep_level, internal_index) == std::make_tuple(&oth.owner, oth.dep_level, oth.internal_index);
        }

        bool operator!=(const const_iterator &oth) const {
          return !(*this == oth);
        }

      private:
        const DepLevelContainer &owner;
        size_t dep_level;
        size_t internal_index;
    };

    const_iterator begin() const {
      size_t begin_idx = 0;
      for (; begin_idx < mapping.size() && mapping[begin_idx].empty(); ++begin_idx) {}
      return const_iterator(*this, begin_idx, 0);
    }

    const_iterator end() const {
      return const_iterator(*this, mapping.size(), 0);
    }

    size_t size() const {
      return cnt;
    }

    size_t empty() const {
      return size() == 0;
    }

    size_t max_dep_level() const {
      return mapping.size();
    }

    const std::vector<VarPtr> & vars_by_dep_level(size_t dep_level) {
      if (dep_level >= max_dep_level()) {
        static const auto EMPTY = std::vector<VarPtr>{};
        return EMPTY;
      }
      return mapping[dep_level];
    }

    void add(VarPtr v) {
      ++cnt;
      auto dep_level = v->dependency_level;
      if (dep_level >= mapping.size()) {
        mapping.resize(dep_level + 1);
      }
      mapping[dep_level].emplace_back(v);
    }

  private:
    friend const_iterator;
    static constexpr auto DEFAULT_MAX_DEP_LEVEL = 8;
    std::vector<std::vector<VarPtr>> mapping;
    size_t cnt;
};

class RawString {
public:
  explicit RawString(vk::string_view str) :
    str(str) {
  }

  void compile(CodeGenerator &W) const;

private:
  std::string str;
};

std::vector<int> compile_arrays_raw_representation(const DepLevelContainer &const_raw_array_vars, CodeGenerator &W);

template <typename Container,
  typename = decltype(std::declval<Container>().begin()),
  typename = decltype(std::declval<Container>().end())>
std::vector<int> compile_raw_data(CodeGenerator &W, const Container &values) {
  std::string raw_data;
  std::vector<int> const_string_shifts(values.size());
  int ii = 0;
  for (const auto &s : values) {
    int shift_to_align = (((int)raw_data.size() + 7) & -8) - (int)raw_data.size();
    if (shift_to_align != 0) {
      raw_data.append(shift_to_align, 0);
    }
    int raw_len = string_raw_len(static_cast<int>(s.size()));
    kphp_assert (raw_len != -1);
    const_string_shifts[ii] = (int)raw_data.size();
    raw_data.append(raw_len, 0);
    int err = string_raw(&raw_data[const_string_shifts[ii]], raw_len, s.c_str(), (int)s.size());
    kphp_assert (err == raw_len);
    ii++;
  }
  if (!raw_data.empty()) {
    W << "alignas(8) static const char raw[] = " << RawString(raw_data) << ";" << NL;
  }
  return const_string_shifts;
}

