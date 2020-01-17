#pragma once
#include <array>

class string_cache {
private:
  struct string_8bytes {
    static constexpr size_t TAIL_SIZE = 8u;

    string::string_inner inner{0, 0, REF_CNT_FOR_CONST};
    char data[TAIL_SIZE]{'\0'};
  };

public:
  static const string_8bytes &empty_string() noexcept {
    static const string_8bytes empty_string_;
    return empty_string_;
  }

  static const string_8bytes &cached_char(char c) noexcept {
    static const auto cached_chars = make_chars();
    return cached_chars[static_cast<uint8_t>(c)];
  }

  static const auto &cached_ints() noexcept {
    static const auto cached_ints = make_ints();
    return cached_ints;
  }

private:
  static std::array<string_8bytes, 256> make_chars() noexcept;
  static std::array<string_8bytes, 10000> make_ints() noexcept;
};

void global_init_string_cache() noexcept;
