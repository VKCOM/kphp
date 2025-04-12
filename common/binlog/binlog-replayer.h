// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "common/kprintf.h"
#include "common/precise-time.h"
#include "common/server/main-binlog.h"
#include "common/type_traits/function_traits.h"
#include "common/wrappers/demangle.h"

#include "common/binlog/kdb-binlog-common.h"

namespace vk {
namespace binlog {

namespace detail {

struct have_get_size_checker {
  template<typename T, typename U>
  struct SFINAE;
  template<typename T>
  static std::true_type test(SFINAE<T, decltype(&T::get_extra_bytes)> *);
  template<typename T>
  static std::false_type test(...);
};

template<typename T>
using have_get_size = decltype(have_get_size_checker::test<T>(0));

struct get_size_helper {
  template<typename T>
  static int get_size_impl(const T &E, std::true_type) {
    return static_cast<int>(sizeof(T)) + E.get_extra_bytes();
  }
  template<typename T>
  static int get_size_impl(const T &, std::false_type) {
    return static_cast<int>(sizeof(T));
  }
  template<typename T>
  static int get_size(const T &E) {
    return get_size_impl(E, have_get_size<T>{});
  }
};

struct have_name_checker {
  template<typename T, typename U>
  struct SFINAE;
  template<typename T>
  static std::true_type test(SFINAE<T, decltype(T::NAME)> *);
  template<typename T>
  static std::false_type test(...);
};

template<typename T>
using have_name = decltype(have_name_checker::test<T>(nullptr));

struct get_name_helper {
  template<typename T>
  static std::string get_name_impl(std::true_type) {
    return T::NAME;
  }
  template<typename T>
  static std::string get_name_impl(std::false_type) {
    return demangle<T>();
  }

  template<typename T>
  static std::string get_name() {
    return get_name_impl<T>(have_name<T>{});
  }
};

template<typename T>
using event_by_handler = vk::decay_function_arg_t<T, 0>;

template<typename ResT>
struct call_handler_dispatcher {
  template<typename Handler, typename T>
  static void call(const Handler &, const T &) {
    static_assert(std::is_same<T, void>{}, "binlog handler should return either void or replay_binlog_result");
  }
};

template<>
struct call_handler_dispatcher<void> {
  template<typename Handler, typename T>
  static replay_binlog_result call(const Handler &handler, const T &E) {
    handler(E);
    return REPLAY_BINLOG_OK;
  }
};

template<>
struct call_handler_dispatcher<replay_binlog_result> {
  template<typename Handler, typename T>
  static replay_binlog_result call(const Handler &handler, const T &E) {
    return handler(E);
  }
};



template<typename Handler, typename T>
replay_binlog_result call_handler(const Handler &handler, const T &E) {
  return call_handler_dispatcher<decltype(handler(E))>::call(handler, E);
}



template<typename T>
std::function<int(const lev_generic &, int)> get_handler_wrapper(T &&handler_) {
  auto handler = std::forward<T>(handler_);
  return [handler](const lev_generic &E, int size) -> int {
    using lev = detail::event_by_handler<T>;
    static_assert(std::is_class<lev>(), "log event must be a structure");
    const auto &EE = reinterpret_cast<const lev &>(E);
    if (size < sizeof(EE)) {
      return REPLAY_BINLOG_NOT_ENOUGH_DATA;
    }
    int real_size = get_size_helper::get_size(EE);
    if (size < real_size) {
      return REPLAY_BINLOG_NOT_ENOUGH_DATA;
    }
    replay_binlog_result res = call_handler(handler, EE);
    if (res != REPLAY_BINLOG_OK) {
      return res;
    }
    return real_size;
  };
}

} // namespace detail

class replayer {
  using handler_func_t = std::function<int(const lev_generic &, int)>;

  struct handler_record {
    explicit handler_record(handler_func_t handler, handler_func_t size_handler, std::string name, lev_type_t base_magic) :
      handler{std::move(handler)},
      size_handler{std::move(size_handler)},
      name{std::move(name)},
      calls{0},
      time_spent{0},
      base_magic{base_magic},
      used_space{0} {}

    int operator()(const lev_generic &E, int size) const {
      double start_time = get_network_time();
      ++calls;
      int res = handler(E, size);
      time_spent += get_network_time() - start_time;
      // if success, handler returns real size of event else error code
      if (res > 0) {
        used_space += res;
      }
      return res;
    }

    int get_event_size(const lev_generic &E, int size) const {
      return size_handler(E, size);
    }

    std::string get_name() const {
      return name;
    }
    double get_time_spent() const {
      return time_spent;
    }
    double get_average_time_spent() const {
      if (calls == 0) {
        return 0;
      }
      return time_spent / (double)calls;
    }
    size_t get_calls() const {
      return calls;
    }

    uint64_t get_used_space() const {
      return used_space;
    }

    handler_func_t handler;
    handler_func_t size_handler;
    std::string name;
    mutable size_t calls;
    mutable double time_spent;
    lev_type_t base_magic;
    mutable uint64_t used_space;
  };

  std::unordered_map<lev_type_t, handler_record> handlers;
  std::function<int(const lev_generic *, int)> default_handler;

  int do_replay(const lev_generic *E, int size, bool just_size) const {
    auto it = handlers.find(E->type);
    if (it == handlers.end()) {
      if (default_handler != nullptr) {
        return default_handler(E, size);
      } else {
        kprintf("unknown log event type %08x at position %lld\n", E->type, log_cur_pos());
        return REPLAY_BINLOG_ERROR;
      }
    }
    return just_size ? it->second.get_event_size(*E, size) : it->second(*E, size);
  }

public:

  explicit replayer(std::function<int(const lev_generic *, int)> default_handler) :
    default_handler(std::move(default_handler)) {}

  explicit replayer() = default;

  bool has_handler(lev_type_t magic) const {
    return handlers.find(magic) != handlers.end();
  }

  int replay(const lev_generic *E, int size) const {
    return do_replay(E, size, false);
  }

  int get_event_size(const lev_generic *E, int size) const {
    return do_replay(E, size, true);
  }

  template<typename T>
  replayer &add_handler(lev_type_t magic, T &&handler, lev_type_t base_magic = 0) {
    if (base_magic == 0) {
      base_magic = magic;
    }
    handler_record h{detail::get_handler_wrapper(std::forward<T>(handler)),
                     detail::get_handler_wrapper([](const detail::event_by_handler<T> &) {}),
                     detail::get_name_helper::get_name<typename detail::event_by_handler<T>>(),
                     base_magic};
    bool inserted = handlers.emplace(magic, std::move(h)).second;
    assert(inserted);
    return *this;
  }

  template<typename T>
  replayer &add_skip_handler() {
    return add_handler([](const T&){});
  }

  template<typename T>
  replayer &add_skip_handler_range() {
    return add_handler_range([](const T&){});
  }

  template<typename T>
  replayer &add_handler(T &&handler) {
    return add_handler(detail::event_by_handler<T>::MAGIC, std::forward<T>(handler));
  }

  template<typename T>
  replayer &add_handler_range(lev_type_t from, lev_type_t to, const T &handler) {
    for (lev_type_t magic = from; magic < to; magic++) {
      add_handler(magic, handler, from);
    }
    return *this;
  }

  template<typename T>
  replayer &add_handler_range(const T &handler) {
    using event_type = detail::event_by_handler<T>;
    return add_handler_range(event_type::MAGIC_BASE, event_type::MAGIC_BASE + event_type::MAGIC_MASK + 1, handler);
  }

  template<typename T>
  static size_t get_event_size(const T &EE) {
    return detail::get_size_helper::get_size(EE);
  }

};

} // namespace binlog
} // namespace vk
