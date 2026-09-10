// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/confdata/confdata-keys.h"
#include "runtime-light/stdlib/confdata/confdata-reader-lease.h"
#include "runtime-light/stdlib/confdata/confdata-storage.h"
#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

namespace {

using kphp::confdata::predefined_wildcards;
using kphp::confdata::predefined_wildcards_error;
using kphp::confdata::section_kind;

using remainder_type = kphp::confdata::key_views::remainder_type;

struct key_sample {
  std::string_view key;
  section_kind kind;
  std::string_view section;
  remainder_type remainder;
};

struct section_sample {
  std::string_view section;
  section_kind kind;
};

struct two_dots_sample {
  key_sample split;
  key_sample reinterpreted;
};

auto check(bool condition, const char* expression, int line) noexcept -> void {
  if (!condition) [[unlikely]] {
    std::ignore = std::fprintf(stderr, "check failed at line %d: %s\n", line, expression);
    std::abort();
  }
}

#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

constexpr auto DEFAULT_ALLOCATOR_SIZE{static_cast<size_t>(1024U * 1024U)};
constexpr auto SHARED_STORAGE_SIZE{static_cast<size_t>(256U * 1024U)};

class wildcard_fixture final {
  alignas(std::max_align_t) std::array<std::byte, SHARED_STORAGE_SIZE> m_memory{};
  kphp::confdata::storage m_storage;

public:
  explicit wildcard_fixture(std::vector<std::string_view> wildcards) noexcept {
    for (const auto wildcard : wildcards) {
      CHECK(kphp::confdata::validate_predefined_wildcard(wildcard).has_value());
    }
    std::ranges::sort(wildcards);
    wildcards.erase(std::ranges::unique(wildcards).begin(), wildcards.end());
    CHECK(m_storage.init(m_memory).has_value());
    CHECK(m_storage.initialize_wildcards(wildcards).has_value());
  }

  ~wildcard_fixture() {
    m_storage.close();
  }

  auto wildcards() const noexcept -> const predefined_wildcards& {
    return m_storage.wildcards();
  }

  auto storage() const noexcept -> const kphp::confdata::storage& {
    return m_storage;
  }
};

auto make_wildcards(std::vector<std::string_view> wildcards) -> wildcard_fixture {
  return wildcard_fixture{std::move(wildcards)};
}

auto initialize_noncanonical(std::span<const std::string_view> wildcards) -> std::expected<void, predefined_wildcards_error> {
  alignas(std::max_align_t) std::array<std::byte, SHARED_STORAGE_SIZE> memory{};
  kphp::confdata::storage storage{};
  CHECK(storage.init(memory).has_value());
  auto result{storage.initialize_wildcards(wildcards)};
  storage.close();
  return result;
}

auto matching(const predefined_wildcards& wildcards, std::string_view key) -> std::vector<std::string_view> {
  std::vector<std::string_view> result{};
  wildcards.for_each_matching_wildcard(key, [&result](std::string_view wildcard) { result.emplace_back(wildcard); });
  return result;
}

auto no_remainder() noexcept -> remainder_type {
  return std::monostate{};
}

auto string_remainder(std::string_view value) noexcept -> remainder_type {
  return value;
}

auto integer_remainder(int64_t value) noexcept -> remainder_type {
  return value;
}

auto check_key(const kphp::confdata::key_views& actual, const key_sample& expected, int line) noexcept -> void {
  if (actual.raw_key() != expected.key || actual.kind() != expected.kind || actual.section() != expected.section || actual.remainder() != expected.remainder)
      [[unlikely]] {
    std::ignore = std::fprintf(stderr, "key check failed at line %d: key -> '%.*s'\n", line, static_cast<int>(expected.key.size()), expected.key.data());
    std::abort();
  }
}

#define CHECK_KEY(actual, expected) check_key(actual, expected, __LINE__)

auto check_split(const key_sample& expected) -> void {
  const auto actual{kphp::confdata::split_key(expected.key)};
  CHECK(actual.has_value());
  CHECK_KEY(*actual, expected);
}

auto check_split(const key_sample& expected, const predefined_wildcards& wildcards) -> void {
  const auto actual{kphp::confdata::split_key(expected.key, wildcards)};
  CHECK(actual.has_value());
  CHECK_KEY(*actual, expected);
}

auto check_two_dots_split(const two_dots_sample& expected) -> void {
  const auto split{kphp::confdata::split_key(expected.split.key)};
  CHECK(split.has_value());
  CHECK_KEY(*split, expected.split);

  const auto reinterpreted{split->reinterpret_two_dots_as_one_dot()};
  CHECK(reinterpreted.has_value());
  CHECK_KEY(*reinterpreted, expected.reinterpreted);
}

auto test_validation_and_formatting() -> void {
  CHECK(kphp::confdata::validate_predefined_wildcard("").error() == predefined_wildcards_error::empty_wildcard);
  CHECK(kphp::confdata::validate_predefined_wildcard("foo.").error() == predefined_wildcards_error::reserved_wildcard);
  CHECK(kphp::confdata::validate_predefined_wildcard("foo.bar.").error() == predefined_wildcards_error::reserved_wildcard);
  CHECK(kphp::confdata::validate_predefined_wildcard("foo").has_value());
  CHECK(kphp::confdata::validate_predefined_wildcard("foo.bar").has_value());
  CHECK(kphp::confdata::validate_predefined_wildcard("foo.bar.baz.").has_value());
  CHECK(kphp::confdata::validate_predefined_wildcard("foo...").has_value());
  CHECK(std::format("{}", predefined_wildcards_error::empty_wildcard) == "empty wildcard");
}

auto test_empty_predefined_wildcards_matrix() -> void {
  const auto fixture{make_wildcards({})};
  const auto& wildcards{fixture.wildcards()};

  for (const std::string_view key : {"", ".", "..", "abc", "abc.", "abc.def", "abc.def.", "abc.def.ghi", "abc.def.ghi."}) {
    CHECK(matching(wildcards, key).empty());
    CHECK(!wildcards.has_matching_wildcard(key));
  }
  CHECK(wildcards.max_matches_per_key() == 0);
  for (const std::string_view wildcard : {"", ".", "abc", "abc.", "abc.def", "abc.def."}) {
    CHECK(!wildcards.contains(wildcard));
  }

  const std::vector<section_sample> sections{
      {"", section_kind::simple_key},
      {"abc", section_kind::simple_key},
      {"abc.def", section_kind::simple_key},
      {"abc.def.ghi", section_kind::simple_key},
      {"abc.def.ghi.", section_kind::simple_key},
      {"abc.def.ghi.jkl", section_kind::simple_key},
      {".", section_kind::one_dot_wildcard},
      {"abc.", section_kind::one_dot_wildcard},
      {"..", section_kind::two_dots_wildcard},
      {"abc..", section_kind::two_dots_wildcard},
      {".abc.", section_kind::two_dots_wildcard},
      {"abc.def.", section_kind::two_dots_wildcard},
  };
  for (const auto& sample : sections) {
    CHECK(kphp::confdata::classify_section(sample.section, wildcards) == sample.kind);
  }
}

auto test_predefined_wildcards_matrix() -> void {
  const auto fixture{make_wildcards({"a", "ab", "abc", "c"})};
  const auto& wildcards{fixture.wildcards()};

  for (const std::string_view key : {"", ".", "..", "xyz", "xyz.abc"}) {
    CHECK(matching(wildcards, key).empty());
  }
  for (const std::string_view key : {"a", "acb", "a.bc", "axyz"}) {
    CHECK(matching(wildcards, key) == (std::vector<std::string_view>{"a"}));
  }
  for (const std::string_view key : {"ab", "abb", "ab.c", "abxyz"}) {
    CHECK(matching(wildcards, key) == (std::vector<std::string_view>{"a", "ab"}));
  }
  for (const std::string_view key : {"abc", "abc.def", "abcdef"}) {
    CHECK(matching(wildcards, key) == (std::vector<std::string_view>{"a", "ab", "abc"}));
  }
  for (const std::string_view key : {"c", "cab", "cccc", "c.axyz"}) {
    CHECK(matching(wildcards, key) == (std::vector<std::string_view>{"c"}));
  }

  CHECK(wildcards.max_matches_per_key() == 3);
  CHECK(wildcards.shortest_matching_wildcard("abc") == "a");
  for (const std::string_view wildcard : {"a", "ab", "abc", "c"}) {
    CHECK(wildcards.contains(wildcard));
  }
  for (const std::string_view wildcard : {"", ".", "a.", "abc.", "abc.def"}) {
    CHECK(!wildcards.contains(wildcard));
  }

  const std::vector<section_sample> sections{
      {"", section_kind::simple_key},
      {"abc.def", section_kind::simple_key},
      {"abc.def.ghi", section_kind::simple_key},
      {"abc.def.ghi.", section_kind::simple_key},
      {"abc.def.ghi.jkl", section_kind::simple_key},
      {".", section_kind::one_dot_wildcard},
      {"abc.", section_kind::one_dot_wildcard},
      {"..", section_kind::two_dots_wildcard},
      {"abc..", section_kind::two_dots_wildcard},
      {".abc.", section_kind::two_dots_wildcard},
      {"abc.def.", section_kind::two_dots_wildcard},
      {"a", section_kind::predefined_wildcard},
      {"ab", section_kind::predefined_wildcard},
      {"abc", section_kind::predefined_wildcard},
      {"c", section_kind::predefined_wildcard},
  };
  for (const auto& sample : sections) {
    CHECK(kphp::confdata::classify_section(sample.section, wildcards) == sample.kind);
  }

  for (const std::string_view key : {"", ".", "b", "ba"}) {
    CHECK(!wildcards.has_matching_wildcard(key));
  }
  for (const std::string_view key : {"ab", "abc", "abc.", "abc.def", "abc.def.", "c.ab", "a.bc", "a", "c", "cxyz"}) {
    CHECK(wildcards.has_matching_wildcard(key));
  }

  CHECK(wildcards.is_top_level_wildcard("a"));
  CHECK(!wildcards.is_top_level_wildcard("ab"));
  CHECK(!wildcards.is_top_level_wildcard("abc"));
  CHECK(wildcards.is_top_level_wildcard("c"));
  CHECK(!wildcards.is_top_level_wildcard("missing"));
}

auto test_shared_storage_index() -> void {
  const auto fixture{make_wildcards({"foo", "foobar", "zip"})};
  kphp::confdata::storage reader{};
  CHECK(reader.open(fixture.storage().memory()).has_value());

  const auto& wildcards{reader.wildcards()};
  CHECK(matching(wildcards, "foobar.value") == (std::vector<std::string_view>{"foo", "foobar"}));
  const auto shortest{wildcards.shortest_matching_wildcard("foobar.value")};
  CHECK(shortest.has_value());
  const auto shared_memory{reader.memory()};
  CHECK(reinterpret_cast<const std::byte*>(shortest->data()) >= shared_memory.data());
  CHECK(reinterpret_cast<const std::byte*>(shortest->data()) < shared_memory.data() + shared_memory.size());
  CHECK(fixture.wildcards().max_matches_per_key() == wildcards.max_matches_per_key());
  reader.close();
}

auto test_noncanonical_wildcards() -> void {
  const std::vector<std::string_view> noncanonical{"b", "a"};
  const auto result{initialize_noncanonical(noncanonical)};
  CHECK(!result.has_value());
  CHECK(result.error() == predefined_wildcards_error::non_canonical_wildcards);
}

auto test_zero_dots_key_matrix() -> void {
  const std::vector<key_sample> samples{
      {"", section_kind::simple_key, "", no_remainder()},
      {"x", section_kind::simple_key, "x", no_remainder()},
      {"1", section_kind::simple_key, "1", no_remainder()},
      {"hello world!", section_kind::simple_key, "hello world!", no_remainder()},
  };
  for (const auto& sample : samples) {
    check_split(sample);
  }
}

auto test_one_dot_empty_remainder_matrix() -> void {
  const std::vector<key_sample> samples{
      {".", section_kind::one_dot_wildcard, ".", string_remainder("")},
      {"x.", section_kind::one_dot_wildcard, "x.", string_remainder("")},
      {"1.", section_kind::one_dot_wildcard, "1.", string_remainder("")},
      {"hello world!.", section_kind::one_dot_wildcard, "hello world!.", string_remainder("")},
  };
  for (const auto& sample : samples) {
    check_split(sample);
  }
}

auto test_one_dot_string_remainder_matrix() -> void {
  const std::vector<key_sample> samples{
      {".g", section_kind::one_dot_wildcard, ".", string_remainder("g")},
      {"x.gg", section_kind::one_dot_wildcard, "x.", string_remainder("gg")},
      {"1.two", section_kind::one_dot_wildcard, "1.", string_remainder("two")},
      {"hello .world!", section_kind::one_dot_wildcard, "hello .", string_remainder("world!")},
      {"big.9223372036854775808", section_kind::one_dot_wildcard, "big.", string_remainder("9223372036854775808")},
      {"small.-9223372036854775809", section_kind::one_dot_wildcard, "small.", string_remainder("-9223372036854775809")},
      {"bad_num0.-0", section_kind::one_dot_wildcard, "bad_num0.", string_remainder("-0")},
      {"bad_num1.1xx", section_kind::one_dot_wildcard, "bad_num1.", string_remainder("1xx")},
      {"bad_num2.012", section_kind::one_dot_wildcard, "bad_num2.", string_remainder("012")},
  };
  for (const auto& sample : samples) {
    check_split(sample);
  }
}

auto test_one_dot_integer_remainder_matrix() -> void {
  const std::vector<key_sample> samples{
      {".123", section_kind::one_dot_wildcard, ".", integer_remainder(123)},
      {"x.-15", section_kind::one_dot_wildcard, "x.", integer_remainder(-15)},
      {"x.48", section_kind::one_dot_wildcard, "x.", integer_remainder(48)},
      {"0.0", section_kind::one_dot_wildcard, "0.", integer_remainder(0)},
      {"max.9223372036854775807", section_kind::one_dot_wildcard, "max.", integer_remainder(std::numeric_limits<int64_t>::max())},
      {"min.-9223372036854775808", section_kind::one_dot_wildcard, "min.", integer_remainder(std::numeric_limits<int64_t>::min())},
  };
  for (const auto& sample : samples) {
    check_split(sample);
  }
}

auto test_two_dots_empty_remainder_matrix() -> void {
  const std::vector<two_dots_sample> samples{
      {{"..", section_kind::two_dots_wildcard, "..", string_remainder("")}, {"..", section_kind::one_dot_wildcard, ".", string_remainder(".")}},
      {{".ab.", section_kind::two_dots_wildcard, ".ab.", string_remainder("")}, {".ab.", section_kind::one_dot_wildcard, ".", string_remainder("ab.")}},
      {{"x..", section_kind::two_dots_wildcard, "x..", string_remainder("")}, {"x..", section_kind::one_dot_wildcard, "x.", string_remainder(".")}},
      {{"x.y.", section_kind::two_dots_wildcard, "x.y.", string_remainder("")}, {"x.y.", section_kind::one_dot_wildcard, "x.", string_remainder("y.")}},
      {{"1..", section_kind::two_dots_wildcard, "1..", string_remainder("")}, {"1..", section_kind::one_dot_wildcard, "1.", string_remainder(".")}},
      {{"1.2.", section_kind::two_dots_wildcard, "1.2.", string_remainder("")}, {"1.2.", section_kind::one_dot_wildcard, "1.", string_remainder("2.")}},
      {{"hello world!..", section_kind::two_dots_wildcard, "hello world!..", string_remainder("")},
       {"hello world!..", section_kind::one_dot_wildcard, "hello world!.", string_remainder(".")}},
      {{"hello.world!.", section_kind::two_dots_wildcard, "hello.world!.", string_remainder("")},
       {"hello.world!.", section_kind::one_dot_wildcard, "hello.", string_remainder("world!.")}},
  };
  for (const auto& sample : samples) {
    check_two_dots_split(sample);
  }
}

auto test_two_dots_string_remainder_matrix() -> void {
  const std::vector<two_dots_sample> samples{
      {{"..g", section_kind::two_dots_wildcard, "..", string_remainder("g")}, {"..g", section_kind::one_dot_wildcard, ".", string_remainder(".g")}},
      {{"x.g.g", section_kind::two_dots_wildcard, "x.g.", string_remainder("g")}, {"x.g.g", section_kind::one_dot_wildcard, "x.", string_remainder("g.g")}},
      {{"1.2.two", section_kind::two_dots_wildcard, "1.2.", string_remainder("two")},
       {"1.2.two", section_kind::one_dot_wildcard, "1.", string_remainder("2.two")}},
      {{"hello. .world!", section_kind::two_dots_wildcard, "hello. .", string_remainder("world!")},
       {"hello. .world!", section_kind::one_dot_wildcard, "hello.", string_remainder(" .world!")}},
      {{"big.num.9223372036854775808", section_kind::two_dots_wildcard, "big.num.", string_remainder("9223372036854775808")},
       {"big.num.9223372036854775808", section_kind::one_dot_wildcard, "big.", string_remainder("num.9223372036854775808")}},
      {{"small.num.-9223372036854775809", section_kind::two_dots_wildcard, "small.num.", string_remainder("-9223372036854775809")},
       {"small.num.-9223372036854775809", section_kind::one_dot_wildcard, "small.", string_remainder("num.-9223372036854775809")}},
      {{"bad_num.0.-0", section_kind::two_dots_wildcard, "bad_num.0.", string_remainder("-0")},
       {"bad_num.0.-0", section_kind::one_dot_wildcard, "bad_num.", string_remainder("0.-0")}},
      {{"bad_num.1.1xx", section_kind::two_dots_wildcard, "bad_num.1.", string_remainder("1xx")},
       {"bad_num.1.1xx", section_kind::one_dot_wildcard, "bad_num.", string_remainder("1.1xx")}},
      {{"bad_num.2.012", section_kind::two_dots_wildcard, "bad_num.2.", string_remainder("012")},
       {"bad_num.2.012", section_kind::one_dot_wildcard, "bad_num.", string_remainder("2.012")}},
  };
  for (const auto& sample : samples) {
    check_two_dots_split(sample);
  }
}

auto test_two_dots_integer_remainder_matrix() -> void {
  const std::vector<two_dots_sample> samples{
      {{"..123", section_kind::two_dots_wildcard, "..", integer_remainder(123)}, {"..123", section_kind::one_dot_wildcard, ".", string_remainder(".123")}},
      {{"x.y.-15", section_kind::two_dots_wildcard, "x.y.", integer_remainder(-15)},
       {"x.y.-15", section_kind::one_dot_wildcard, "x.", string_remainder("y.-15")}},
      {{"x..48", section_kind::two_dots_wildcard, "x..", integer_remainder(48)}, {"x..48", section_kind::one_dot_wildcard, "x.", string_remainder(".48")}},
      {{"0.-1.0", section_kind::two_dots_wildcard, "0.-1.", integer_remainder(0)}, {"0.-1.0", section_kind::one_dot_wildcard, "0.", string_remainder("-1.0")}},
      {{"max.n.2147483647", section_kind::two_dots_wildcard, "max.n.", integer_remainder(2147483647)},
       {"max.n.2147483647", section_kind::one_dot_wildcard, "max.", string_remainder("n.2147483647")}},
      {{"min.n.-2147483648", section_kind::two_dots_wildcard, "min.n.", integer_remainder(-2147483648LL)},
       {"min.n.-2147483648", section_kind::one_dot_wildcard, "min.", string_remainder("n.-2147483648")}},
  };
  for (const auto& sample : samples) {
    check_two_dots_split(sample);
  }
}

auto test_explicit_predefined_wildcard_matrix() -> void {
  struct sample {
    size_t wildcard_size;
    key_sample expected;
  };
  const std::vector<sample> samples{
      {3, {"abcd", section_kind::predefined_wildcard, "abc", string_remainder("d")}},
      {3, {"123abc", section_kind::predefined_wildcard, "123", string_remainder("abc")}},
      {3, {"abc123", section_kind::predefined_wildcard, "abc", integer_remainder(123)}},
      {1, {"abc123", section_kind::predefined_wildcard, "a", string_remainder("bc123")}},
  };
  for (const auto& sample : samples) {
    const auto actual{kphp::confdata::split_key_with_predefined_wildcard(sample.expected.key, sample.wildcard_size)};
    CHECK(actual.has_value());
    CHECK_KEY(*actual, sample.expected);
  }
}

auto test_automatic_predefined_wildcard_matrix() -> void {
  // Implicit one-dot/two-dot sections are deliberately excluded from the configured index.
  const auto fixture{make_wildcards({"abc", "abc.xyz", "cde", "cd"})};
  const auto& wildcards{fixture.wildcards()};
  const std::vector<key_sample> samples{
      {"abc", section_kind::predefined_wildcard, "abc", string_remainder("")},
      {"abc.", section_kind::predefined_wildcard, "abc", string_remainder(".")},
      {"abcd", section_kind::predefined_wildcard, "abc", string_remainder("d")},
      {"abc123", section_kind::predefined_wildcard, "abc", integer_remainder(123)},
      {"abc.xyz", section_kind::predefined_wildcard, "abc", string_remainder(".xyz")},
      {"abc.xyz.uvz", section_kind::predefined_wildcard, "abc", string_remainder(".xyz.uvz")},
      {"cd", section_kind::predefined_wildcard, "cd", string_remainder("")},
      {"cdxxx", section_kind::predefined_wildcard, "cd", string_remainder("xxx")},
      {"cde.xyz", section_kind::predefined_wildcard, "cd", string_remainder("e.xyz")},
      {"a", section_kind::simple_key, "a", no_remainder()},
      {"ab", section_kind::simple_key, "ab", no_remainder()},
      {"hello world", section_kind::simple_key, "hello world", no_remainder()},
      {"foo", section_kind::simple_key, "foo", no_remainder()},
      {"foo.", section_kind::one_dot_wildcard, "foo.", string_remainder("")},
      {"foo.bar", section_kind::one_dot_wildcard, "foo.", string_remainder("bar")},
      {"hello.world", section_kind::one_dot_wildcard, "hello.", string_remainder("world")},
      {"foo.bar.", section_kind::two_dots_wildcard, "foo.bar.", string_remainder("")},
      {"foo.bar.baz", section_kind::two_dots_wildcard, "foo.bar.", string_remainder("baz")},
      {"hello.wo.ld", section_kind::two_dots_wildcard, "hello.wo.", string_remainder("ld")},
  };
  for (const auto& sample : samples) {
    check_split(sample, wildcards);
  }
}

auto test_key_splitting_errors() -> void {
  const auto invalid_wildcard_length{kphp::confdata::split_key_with_predefined_wildcard("abc", 0)};
  CHECK(!invalid_wildcard_length.has_value());
  CHECK(invalid_wildcard_length.error() == kphp::confdata::split_error::invalid_predefined_wildcard_length);

  const auto excessive_wildcard_length{kphp::confdata::split_key_with_predefined_wildcard("abc", 4)};
  CHECK(!excessive_wildcard_length.has_value());
  CHECK(excessive_wildcard_length.error() == kphp::confdata::split_error::invalid_predefined_wildcard_length);

  const auto simple{kphp::confdata::split_key("simple")};
  CHECK(simple.has_value());
  const auto not_two_dots{simple->reinterpret_two_dots_as_one_dot()};
  CHECK(!not_two_dots.has_value());
  CHECK(not_two_dots.error() == kphp::confdata::split_error::not_a_two_dots_key);

  std::string oversized_key{};
  oversized_key.assign(kphp::confdata::MAX_KEY_LENGTH + 1, 'x');
  const auto oversized{kphp::confdata::split_key(oversized_key)};
  CHECK(!oversized.has_value());
  CHECK(oversized.error() == kphp::confdata::split_error::key_too_long);
}

auto test_reader_lease_handshake() -> void {
  const kphp::confdata::reader_lease empty{};
  CHECK(!empty.is_valid());
  CHECK(!kphp::confdata::reader_lease::create("", 3).has_value());
  CHECK(!kphp::confdata::reader_lease::create("confdata", kphp::confdata::storage::INVALID_SAMPLE_ID).has_value());

  const auto lease{kphp::confdata::reader_lease::create("kphp-confdata", 7)};
  CHECK(lease.has_value());
  CHECK(lease->is_valid());
  CHECK(lease->sample_id() == 7);
  CHECK(lease->shared_memory_name() == "kphp-confdata");
}

} // namespace

extern "C" void* k2_alloc(size_t size, size_t align) {
  const auto actual_align{std::max(align, alignof(std::max_align_t))};
  const auto actual_size{(size + actual_align - 1) / actual_align * actual_align};
  return std::aligned_alloc(actual_align, actual_size);
}

extern "C" void* k2_realloc(void* memory, size_t new_size) {
  return std::realloc(memory, new_size);
}

extern "C" void k2_free(void* memory) {
  std::free(memory);
}

extern "C" void k2_log(size_t /*level*/, size_t /*len*/, const char* /*msg*/, size_t /*kv_count*/, const LogKeyValuePair* /*kv_pairs*/) {}

extern "C" void k2_exit(int32_t /*exit_code*/) {
  std::abort();
}

void runtime_error(const char* /*unused*/, ...) {}

void php_warning(const char* /*unused*/, ...) {}

void php_error(const char* /*unused*/, ...) {}

[[noreturn]] void critical_error_handler() {
  std::abort();
}

[[noreturn]] void php_assert__(const char* /*unused*/, const char* /*unused*/, int /*unused*/) {
  std::abort();
}

auto AllocatorState::get() noexcept -> const AllocatorState& {
  static AllocatorState allocator_state{DEFAULT_ALLOCATOR_SIZE, DEFAULT_ALLOCATOR_SIZE, 0};
  return allocator_state;
}

auto main() -> int {
  test_validation_and_formatting();
  test_empty_predefined_wildcards_matrix();
  test_predefined_wildcards_matrix();
  test_shared_storage_index();
  test_noncanonical_wildcards();
  test_zero_dots_key_matrix();
  test_one_dot_empty_remainder_matrix();
  test_one_dot_string_remainder_matrix();
  test_one_dot_integer_remainder_matrix();
  test_two_dots_empty_remainder_matrix();
  test_two_dots_string_remainder_matrix();
  test_two_dots_integer_remainder_matrix();
  test_explicit_predefined_wildcard_matrix();
  test_automatic_predefined_wildcard_matrix();
  test_key_splitting_errors();
  test_reader_lease_handshake();
  RuntimeAllocator::get().free();
  return 0;
}
