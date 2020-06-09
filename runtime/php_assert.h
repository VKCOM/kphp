#pragma once

#include <cstdio>
#include <unistd.h>

#include "common/wrappers/likely.h"
#include "common/mixin/not_copyable.h"

extern int die_on_fail;

extern const char *engine_tag;
extern const char *engine_pid;
extern int release_version;

extern int php_disable_warnings;
extern int php_warning_level;
extern int php_warning_minimum_level;

void php_warning(char const *message, ...) __attribute__ ((format (printf, 1, 2)));
void php_out_of_memory_warning(char const *message, ...) __attribute__ ((format (printf, 1, 2)));

void php_assert__(const char *msg, const char *file, int line) __attribute__((noreturn));
void raise_php_assert_signal__();

#define php_assert(EX) do {                          \
  if (unlikely(!(EX))) {                             \
    php_assert__ (#EX, __FILE__, __LINE__);          \
  }                                                  \
} while(0)

#define php_critical_error(format, ...) do {                                                              \
  php_warning ("Critical error \"" format "\" in file %s on line %d", ##__VA_ARGS__, __FILE__, __LINE__); \
  raise_php_assert_signal__();                                                                            \
  fprintf (stderr, "_exiting in php_critical_error\n");                                                   \
  _exit (1);                                                                                              \
} while(0)

class KphpErrorContext: vk::not_copyable {
public:
  static KphpErrorContext& get();

  bool tags_are_set() const {
    return tags_buffer[0] != '\0';
  }

  bool extra_info_is_set() const {
    return extra_info_buffer[0] != '\0';
  }

  void set_extra_info(const char *ptr, size_t size);
  void set_tags(const char *ptr, size_t size);
  void reset();

  const char *tags_c_str() const {
    return tags_buffer;
  }

  const char *extra_info_c_str() const {
    return extra_info_buffer;
  }
private:
  KphpErrorContext() = default;
  char tags_buffer[10 * 1024]{0};
  char extra_info_buffer[10 * 1024]{0};
};
