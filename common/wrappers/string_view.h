// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <ostream>
#include <string>

#if __cplusplus > 201703
#include <string_view>
namespace vk {
using std::string_view;
}

inline std::string operator+(std::string s, std::string_view sv) {
  return s += sv;
}
#else
namespace vk {
class string_view {
public:
  using const_iterator = const char*;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<iterator>;
  using reverse_iterator = const_reverse_iterator;

  static constexpr auto npos = std::string::npos;

  constexpr string_view(const char* data, size_t count)
      : _data{data},
        _count{count} {}

  constexpr string_view(const char* data, const char* data_end)
      : _data{data},
        _count{static_cast<size_t>(data_end - data)} {}

  constexpr string_view(const char* data)
      : string_view{data, std::char_traits<char>::length(data)} {}

  template<class Alloc>
  constexpr string_view(const std::basic_string<char, std::char_traits<char>, Alloc>& s)
      : string_view{s.data(), s.size()} {}

#if __cplusplus >= 201703
  constexpr string_view(std::string_view sv)
      : string_view{sv.data(), sv.size()} {}
#endif

  constexpr string_view()
      : string_view{nullptr, nullptr} {}

  const_iterator data() const noexcept {
    return _data;
  }

  const_iterator begin() const noexcept {
    return data();
  }

  const_iterator end() const noexcept {
    return begin() + size();
  }

  const_reverse_iterator rbegin() const noexcept {
    return std::reverse_iterator<iterator>(end());
  }

  const_reverse_iterator rend() const noexcept {
    return std::reverse_iterator<iterator>(begin());
  }

  char front() const noexcept {
    return *begin();
  }

  char back() const noexcept {
    return *rbegin();
  }

  size_t size() const noexcept {
    return _count;
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  void remove_suffix(size_t n) noexcept {
    _count -= n;
  }

  void remove_prefix(size_t n) noexcept {
    _data += n;
    _count -= n;
  }

  bool starts_with(string_view other) const noexcept {
    return other.size() > size() ? false : string_view(data(), other.size()) == other;
  }

  bool ends_with(string_view other) const noexcept {
    return other.size() > size() ? false : string_view(data() + size() - other.size(), other.size()) == other;
  }

  template<typename Alloc>
  explicit operator std::basic_string<char, std::char_traits<char>, Alloc>() const {
    return {data(), size()};
  }

#if __cplusplus >= 201703
  operator std::string_view() const {
    return {data(), size()};
  }
#endif

  bool operator<(string_view other) const noexcept {
    auto cmp = memcmp(data(), other.data(), std::min(size(), other.size()));
    if (cmp != 0) {
      return cmp < 0;
    }
    return size() < other.size();
  }

  friend bool operator==(string_view lhs, string_view rhs) noexcept {
    if (lhs.empty()) {
      return rhs.empty();
    }
    return lhs.size() == rhs.size() && memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
  }

  friend bool operator!=(string_view lhs, string_view rhs) noexcept {
    return !(lhs == rhs);
  }

  size_t hash_code() const noexcept {
#if defined __GLIBCXX__
    return std::_Hash_impl::hash(data(), size());
#elif defined _LIBCPP_VERSION
    return std::__do_string_hash(begin(), end());
#else
#error only libc++ and libstdc++ are supported
#endif
  }

  string_view substr(size_t pos, size_t count = npos) const noexcept {
    assert(pos <= size());
    if (count == npos || pos + count >= _count) {
      return {begin() + pos, end()};
    }
    return {begin() + pos, begin() + pos + count};
  }

  size_t find(string_view needle, size_t pos = 0) const noexcept {
    assert(pos <= size());

    if (needle.empty()) {
      return pos;
    }
    if (empty()) {
      return std::string::npos;
    }
    const auto* ans = static_cast<const char*>(memmem(data() + pos, size() - pos, needle.data(), needle.size()));
    if (ans) {
      return ans - data();
    } else {
      return npos;
    }
  }

  size_t find(char c, size_t pos = 0) const noexcept {
    assert(pos <= size());
    if (empty()) {
      return std::string::npos;
    }
    const auto* ans = static_cast<const char*>(memchr(data() + pos, c, size() - pos));
    if (ans) {
      return ans - data();
    } else {
      return npos;
    }
  }

  size_t rfind(string_view needle, size_t pos = npos) const noexcept {
    if (needle.empty()) {
      if (empty()) {
        return 0;
      }
      return pos <= size() ? pos : npos;
    }

    if (needle.size() <= size()) {
      const auto* end_it = begin() + (std::min(size() - needle.size(), pos) + needle.size());
      const auto* found_it = std::find_end(begin(), end_it, needle.begin(), needle.end());

      if (found_it != end_it) {
        return std::distance(begin(), found_it);
      }
    }
    return npos;
  }

  size_t rfind(char c, size_t pos = npos) const noexcept {
    return rfind(string_view{std::addressof(c), 1}, pos);
  }

  size_t find_first_of(string_view str, size_t pos = 0) const noexcept {
    for (; !str.empty() && pos < size(); ++pos) {
      if (npos != str.find((*this)[pos])) {
        return pos;
      }
    }
    return npos;
  }

  size_t find_first_not_of(string_view str, size_t pos = 0) const noexcept {
    for (; pos < size(); ++pos) {
      if (str.find((*this)[pos]) == npos) {
        return pos;
      }
    }
    return npos;
  }

  char operator[](size_t idx) const noexcept {
    return data()[idx];
  }

private:
  const char* _data = nullptr;
  size_t _count = 0;
};

inline std::string operator+(std::string s, string_view sv) {
  return s.append(sv.begin(), sv.end());
}

inline std::ostream& operator<<(std::ostream& os, string_view sv) {
  return os.write(sv.data(), sv.size());
}

inline string_view operator"" _sv(const char* s, size_t size) {
  return {s, size};
}

template<class Alloc>
bool operator<(string_view lhs, const std::basic_string<char, std::char_traits<char>, Alloc>& rhs) {
  return lhs < string_view(rhs);
}

template<class Alloc>
bool operator<(const std::basic_string<char, std::char_traits<char>, Alloc>& lhs, string_view rhs) {
  return string_view(lhs) < rhs;
}

} // namespace vk

namespace std {
template<>
class hash<vk::string_view> {
public:
  size_t operator()(vk::string_view sv) const noexcept {
    return sv.hash_code();
  }
};
} // namespace std
#endif // __cplusplus > 201703
