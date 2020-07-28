#pragma once
#include <array>

class string_cache {
private:
  struct string_8bytes {
    static constexpr size_t TAIL_SIZE = 8u;

    constexpr explicit string_8bytes(char c) :
      inner{1, 1, ExtraRefCnt::for_global_const},
      data{c, '\0'} {
    }

    template<size_t... Digits>
    constexpr explicit string_8bytes(std::index_sequence<Digits...>) :
      inner{sizeof...(Digits), sizeof...(Digits), ExtraRefCnt::for_global_const},
      data{static_cast<char>('0' + Digits)..., '\0'} {
    }

    constexpr string_8bytes() = default;

    string::string_inner inner{0, 0, ExtraRefCnt::for_global_const};
    char data[TAIL_SIZE]{'\0'};
  };

  // у gcc9.2 какие-то сложности с копированием вложенных массивов, он генерирует warnings о том, что строка не null terminated
  // поэтому make функции не очень работают (хотя для чисел, почему-то работает, хз..)
  using single_char_storage = std::array<string_cache::string_8bytes, 256>;
  struct single_char_storage_hack : single_char_storage {
  private:
    template<size_t... Chars>
    constexpr single_char_storage_hack(std::index_sequence<Chars...>) noexcept :
      single_char_storage{
        {
          string_cache::string_8bytes(static_cast<char>(Chars))...
        }
      } {}

  public:
    constexpr single_char_storage_hack() noexcept :
      single_char_storage_hack{std::make_index_sequence<std::tuple_size<single_char_storage>{}>{}} {
    }
  };

  template<size_t Rem, size_t... Digits>
  struct constexpr_number_to_string : constexpr_number_to_string<Rem / 10, Rem % 10, Digits...> {
  };

  template<size_t... Digits>
  struct constexpr_number_to_string<0, Digits...> {
    static constexpr string_8bytes create() noexcept {
      return sizeof...(Digits)
             ? string_8bytes{std::index_sequence<Digits...>{}}
             : string_8bytes{std::index_sequence<0>{}};
    }
  };

  template<size_t... Ints>
  static constexpr auto constexpr_make_ints(std::index_sequence<Ints...>) noexcept {
    // разбиваем на 10 частей, иначе это компилируется слижком долго
    return std::array<string_cache::string_8bytes, 10 * sizeof...(Ints)>{
      {
        constexpr_number_to_string<0 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<1 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<2 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<3 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<4 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<5 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<6 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<7 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<8 * sizeof...(Ints) + Ints>::create()...,
        constexpr_number_to_string<9 * sizeof...(Ints) + Ints>::create()...
      }};
  }

  static constexpr int64_t small_int_max() noexcept { return 100; }

  static constexpr auto constexpr_make_small_ints() noexcept {
    return constexpr_make_ints(std::make_index_sequence<small_int_max() / 10>{});
  }

  static constexpr auto constexpr_make_large_ints() noexcept;
  static const string::string_inner &cached_large_int(int64_t i) noexcept;

public:
  static const string::string_inner &empty_string() noexcept {
    static constexpr string_8bytes empty_string;
    return empty_string.inner;
  }

  static const string::string_inner &cached_char(char c) noexcept {
    static constexpr single_char_storage_hack constexpr_char_cache;
    return constexpr_char_cache[static_cast<uint8_t>(c)].inner;
  }

  static const string::string_inner &cached_int(int64_t i) noexcept {
    // constexpr_make_small_ints генерирует числа от 0 до 99 (small_int_max - 1),
    // сделанно это для того, что бы сильно не замедлять компиляцию
    // числа от 100 (small_int_max) до 9999 (cached_int_max - 1) лежат в cached_large_int
    static constexpr auto small_int_cache = constexpr_make_small_ints();
    return i < static_cast<int>(small_int_cache.size()) ? small_int_cache[i].inner : cached_large_int(i);
  }

  static constexpr int64_t cached_int_max() noexcept { return 10000; }
};
