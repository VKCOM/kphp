// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#pragma once
#include "compiler/data/data_ptr.h"
#include "common/wrappers/string_view.h"


class PerformanceInspections {
public:
  enum Inspections : uint8_t {
    no_inspections = 0,
    implicit_array_cast = (1 << 0),
    array_merge_into = (1 << 1),
    all_inspections = implicit_array_cast | array_merge_into,
  };

  void add_from_php_doc(vk::string_view php_doc_tag);

  bool has_their_own_inspections() const noexcept {
    return !inspections_are_merged_ && (enabled_inspections_ || disabled_inspections_);
  }

  std::underlying_type_t<Inspections> inspections() const noexcept {
    return enabled_inspections_ & (~disabled_inspections_);
  }

  std::underlying_type_t<Inspections> disabled_inspections() const noexcept {
    return disabled_inspections_;
  }

  enum class InheritStatus {
    ok,
    no_need,
    conflict
  };
  std::pair<InheritStatus, Inspections> merge_with_caller(PerformanceInspections caller_inspections) noexcept;

  static vk::string_view inspection2string(Inspections inspection) noexcept;

private:
  std::underlying_type_t<Inspections> enabled_inspections_{0};
  std::underlying_type_t<Inspections> disabled_inspections_{0};

  bool inspections_are_merged_{false};
};
