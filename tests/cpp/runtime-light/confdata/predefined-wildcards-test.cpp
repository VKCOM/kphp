// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "runtime-light/components/confdata/state/predefined-wildcards-builder.h"
#include "runtime-light/stdlib/confdata/confdata-keys.h"
#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

namespace {

using kphp::confdata::predefined_wildcards;
using kphp::confdata::predefined_wildcards_error;

auto check(bool condition, const char* expression, int line) noexcept -> void {
  if (!condition) [[unlikely]] {
    std::ignore = std::fprintf(stderr, "check failed at line %d: %s\n", line, expression);
    std::abort();
  }
}

#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

auto make_metadata(std::vector<std::string_view> wildcards, std::vector<std::byte>& storage) -> predefined_wildcards {
  for (const auto wildcard : wildcards) {
    CHECK(kphp::confdata::validate_predefined_wildcard(wildcard).has_value());
  }
  std::ranges::sort(wildcards);
  wildcards.erase(std::ranges::unique(wildcards).begin(), wildcards.end());
  const auto metadata_size{kphp::confdata::predefined_wildcards_metadata_size(wildcards)};
  CHECK(metadata_size.has_value());
  storage.resize(*metadata_size);
  const auto metadata{kphp::confdata::write_predefined_wildcards(storage, wildcards)};
  CHECK(metadata.has_value());
  return *metadata;
}

auto matching(const predefined_wildcards& wildcards, std::string_view key) -> std::vector<std::string_view> {
  std::vector<std::string_view> result{};
  wildcards.for_each_matching_wildcard(key, [&result](std::string_view wildcard) { result.emplace_back(wildcard); });
  return result;
}

auto test_validation_and_formatting() -> void {
  CHECK(kphp::confdata::validate_predefined_wildcard("").error() == predefined_wildcards_error::empty_wildcard);
  CHECK(kphp::confdata::validate_predefined_wildcard("foo.").error() == predefined_wildcards_error::reserved_wildcard);
  CHECK(kphp::confdata::validate_predefined_wildcard("foo.bar.").error() == predefined_wildcards_error::reserved_wildcard);
  CHECK(kphp::confdata::validate_predefined_wildcard("foo").has_value());
  CHECK(kphp::confdata::validate_predefined_wildcard("foo...").has_value());
  CHECK(std::format("{}", predefined_wildcards_error::empty_wildcard) == "empty wildcard");
}

auto test_empty_metadata() -> void {
  std::vector<std::byte> storage{};
  const auto wildcards{make_metadata({}, storage)};

  CHECK(wildcards.max_matches_per_key() == 0);
  CHECK(!wildcards.has_matching_wildcard("anything"));
  CHECK(!storage.empty());
}

auto test_indexed_prefix_groups() -> void {
  std::vector<std::byte> storage{};
  const auto wildcards{make_metadata({"abc", "a", "c", "ab", "ca", "d"}, storage)};

  CHECK(matching(wildcards, "abc.def") == (std::vector<std::string_view>{"a", "ab", "abc"}));
  CHECK(matching(wildcards, "cab") == (std::vector<std::string_view>{"c", "ca"}));
  CHECK(matching(wildcards, "d") == (std::vector<std::string_view>{"d"}));
  CHECK(matching(wildcards, "b").empty());

  CHECK(wildcards.max_matches_per_key() == 3);
  CHECK(wildcards.shortest_matching_wildcard("abc") == "a");
  CHECK(wildcards.contains("abc"));
  CHECK(!wildcards.contains("ac"));
  CHECK(wildcards.is_top_level_wildcard("a"));
  CHECK(!wildcards.is_top_level_wildcard("ab"));
  CHECK(!wildcards.is_top_level_wildcard("missing"));
}

auto test_relocation() -> void {
  std::vector<std::byte> original_storage{};
  const auto original{make_metadata({"foo", "foobar", "zip"}, original_storage)};
  std::vector<std::byte> relocated_storage{original_storage};

  const auto relocated{kphp::confdata::open_predefined_wildcards(relocated_storage)};
  CHECK(relocated.has_value());
  CHECK(matching(*relocated, "foobar.value") == (std::vector<std::string_view>{"foo", "foobar"}));
  const auto shortest{relocated->shortest_matching_wildcard("foobar.value")};
  CHECK(shortest.has_value());
  const auto relocated_bytes{std::as_bytes(std::span{relocated_storage})};
  CHECK(reinterpret_cast<const std::byte*>(shortest->data()) >= relocated_bytes.data());
  CHECK(reinterpret_cast<const std::byte*>(shortest->data()) < relocated_bytes.data() + relocated_bytes.size());
  CHECK(original.max_matches_per_key() == relocated->max_matches_per_key());
}

auto test_invalid_metadata() -> void {
  const std::vector<std::string_view> noncanonical{"b", "a"};
  CHECK(kphp::confdata::predefined_wildcards_metadata_size(noncanonical).error() == predefined_wildcards_error::non_canonical_wildcards);

  std::vector<std::byte> storage{};
  static_cast<void>(make_metadata({"abc"}, storage));
  storage.front() ^= std::byte{1};
  const auto corrupted{kphp::confdata::open_predefined_wildcards(storage)};
  CHECK(!corrupted.has_value());
  CHECK(corrupted.error() == predefined_wildcards_error::invalid_metadata);

  std::vector<std::byte> misaligned_storage(storage.size() + 1);
  const auto misaligned{kphp::confdata::open_predefined_wildcards(std::span<const std::byte>{misaligned_storage}.subspan(1))};
  CHECK(!misaligned.has_value());
  CHECK(misaligned.error() == predefined_wildcards_error::misaligned_buffer);
}

auto test_key_splitting() -> void {
  using kphp::confdata::section_kind;

  const auto simple{kphp::confdata::split_key("simple")};
  CHECK(simple.has_value());
  CHECK(simple->kind() == section_kind::simple_key);
  CHECK(simple->section() == "simple");
  CHECK(std::holds_alternative<std::monostate>(simple->remainder()));

  const auto one_dot_integer{kphp::confdata::split_key("foo.-15")};
  CHECK(one_dot_integer.has_value());
  CHECK(one_dot_integer->kind() == section_kind::one_dot_wildcard);
  CHECK(one_dot_integer->section() == "foo.");
  CHECK(std::get<int64_t>(one_dot_integer->remainder()) == -15);

  const auto one_dot_string{kphp::confdata::split_key("foo.012")};
  CHECK(one_dot_string.has_value());
  CHECK(std::get<std::string_view>(one_dot_string->remainder()) == "012");

  const auto two_dots{kphp::confdata::split_key("foo.bar.123")};
  CHECK(two_dots.has_value());
  CHECK(two_dots->kind() == section_kind::two_dots_wildcard);
  CHECK(two_dots->section() == "foo.bar.");
  CHECK(std::get<int64_t>(two_dots->remainder()) == 123);
  const auto one_dot_reinterpretation{two_dots->reinterpret_two_dots_as_one_dot()};
  CHECK(one_dot_reinterpretation.has_value());
  CHECK(one_dot_reinterpretation->kind() == section_kind::one_dot_wildcard);
  CHECK(one_dot_reinterpretation->section() == "foo.");
  CHECK(std::get<std::string_view>(one_dot_reinterpretation->remainder()) == "bar.123");

  std::vector<std::byte> storage{};
  const auto wildcards{make_metadata({"abc"}, storage)};
  const auto predefined{kphp::confdata::split_key("abc123", wildcards)};
  CHECK(predefined.has_value());
  CHECK(predefined->kind() == section_kind::predefined_wildcard);
  CHECK(predefined->section() == "abc");
  CHECK(std::get<int64_t>(predefined->remainder()) == 123);

  const auto invalid_wildcard_length{kphp::confdata::split_key_with_predefined_wildcard("abc", 0)};
  CHECK(!invalid_wildcard_length.has_value());
  CHECK(invalid_wildcard_length.error() == kphp::confdata::split_error::invalid_predefined_wildcard_length);

  std::string oversized_key{};
  oversized_key.assign(kphp::confdata::MAX_KEY_LENGTH + 1, 'x');
  const auto oversized{kphp::confdata::split_key(oversized_key)};
  CHECK(!oversized.has_value());
  CHECK(oversized.error() == kphp::confdata::split_error::key_too_long);
}

} // namespace

auto main() -> int {
  test_validation_and_formatting();
  test_empty_metadata();
  test_indexed_prefix_groups();
  test_relocation();
  test_invalid_metadata();
  test_key_splitting();
  return 0;
}
