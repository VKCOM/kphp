// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/vkext.h"

#include "common/wrappers/string_view.h"
#include "flex/flex.h"
#include "runtime/php_assert.h"

#define BUFF_LEN (1 << 16)
static char buff[BUFF_LEN];

string f$vk_flex(const string &name, const string &case_name, int64_t sex, const string &type, int64_t lang_id) noexcept {
  constexpr size_t ERROR_MSG_BUG_SIZE = 1000;
  static char ERROR_MSG_BUF[ERROR_MSG_BUG_SIZE] = {'\0'};
  ERROR_MSG_BUF[0] = '\0';
  vk::string_view res = flex(vk::string_view{name.c_str(), name.size()}, vk::string_view{case_name.c_str(), case_name.size()}, sex == 1,
                             vk::string_view{type.c_str(), type.size()}, lang_id, buff, ERROR_MSG_BUF, ERROR_MSG_BUG_SIZE);
  if (ERROR_MSG_BUF[0] != '\0') {
    php_warning("%s", ERROR_MSG_BUF);
  }
  if (res.data() == name.c_str()) {
    return name;
  }
  return string{res.data(), static_cast<string::size_type>(res.size())};
}
