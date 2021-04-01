// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/performance-inspections.h"

#include "common/wrappers/to_array.h"
#include "compiler/utils/string-utils.h"

namespace {

constexpr auto get_all_inspections() noexcept {
  return vk::to_array<std::pair<PerformanceInspections::Inspections, const char *>>(
    {
      {PerformanceInspections::array_merge_into,           "array-merge-into"},
      {PerformanceInspections::array_reserve,              "array-reserve"},
      {PerformanceInspections::constant_execution_in_loop, "constant-execution-in-loop"},
      {PerformanceInspections::implicit_array_cast,        "implicit-array-cast"},
      {PerformanceInspections::all_inspections,            "all"},
    });
}

PerformanceInspections::Inspections string2inspection(vk::string_view str_inspection) {
  for (auto i : get_all_inspections()) {
    if (i.second == str_inspection) {
      return i.first;
    }
  }
  throw std::runtime_error{"unknown performance inspection '" + str_inspection + "'"};
}

} // namespace

PerformanceInspections::PerformanceInspections(Inspections enabled) noexcept
  : enabled_inspections_(enabled) {
}

void PerformanceInspections::set_from_php_doc(vk::string_view php_doc_tag) {
  const auto raw_inspections = split_skipping_delimeters(php_doc_tag);
  if (raw_inspections.empty()) {
    throw std::runtime_error{"there are no any inspection"};
  }

  std::underlying_type_t<Inspections> enabled_inspections = 0;
  std::underlying_type_t<Inspections> disabled_inspections = 0;
  for (auto inspection : raw_inspections) {
    if (inspection.starts_with("!")) {
      inspection.remove_prefix(1);
      disabled_inspections |= string2inspection(inspection);
    } else {
      enabled_inspections |= string2inspection(inspection);
    }
  }
  enabled_inspections_ = enabled_inspections;
  disabled_inspections_ = disabled_inspections;
}

std::pair<PerformanceInspections::InheritStatus, PerformanceInspections::Inspections>
PerformanceInspections::merge_with_caller(PerformanceInspections caller_inspections) noexcept {
  if (has_their_own_inspections()) {
    return {InheritStatus::no_need, no_inspections};
  }

  if ((enabled_inspections_ & caller_inspections.enabled_inspections_) == caller_inspections.enabled_inspections_ &&
      (disabled_inspections_ == caller_inspections.disabled_inspections_)) {
    return {InheritStatus::no_need, no_inspections};
  }

  if (std::underlying_type_t<Inspections> conflicts = (inspections() & caller_inspections.disabled_inspections_) |
                                                      (disabled_inspections_ & caller_inspections.inspections())) {
    const auto first_conflict = static_cast<Inspections>((1 << (__builtin_ffsll(conflicts) - 1)));
    return {InheritStatus::conflict, first_conflict};
  }

  inspections_are_merged_ = true;
  enabled_inspections_ |= caller_inspections.enabled_inspections_;
  disabled_inspections_ |= caller_inspections.disabled_inspections_;
  return {InheritStatus::ok, no_inspections};
}

vk::string_view PerformanceInspections::inspection2string(Inspections inspection) noexcept {
  for (auto i : get_all_inspections()) {
    if (i.first == inspection) {
      return i.second;
    }
  }
  return {};
}
