// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdlib>
#include <unistd.h>

#include "runtime/env.h"

static bool valid_environment_name(const char *name, const char *end) noexcept {
  return std::find_if(name, end, [](char s) { return vk::any_of_equal(s, ' ', '.', '['); }) == end;
}

#ifdef __APPLE__
extern char **environ;
#endif

array<string> f$getenv() noexcept {
  array<string> res;
  for (char **env = environ; env && *env; ++env) {
    const char *ptr = std::strchr(*env, '=');
    // malformed entry?
    if (ptr && ptr != *env && valid_environment_name(*env, ptr)) {
      const string key{*env, static_cast<uint32_t>(ptr - *env)};
      string value{++ptr};
      res.set_value(key, std::move(value));
    }
  }
  return res;
}

Optional<string> f$getenv(const string &varname, bool /*local_only*/) noexcept {
  const char *env = std::getenv(varname.c_str());
  return env ? Optional<string>{env} : false;
}
