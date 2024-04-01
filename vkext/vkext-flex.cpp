// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "vkext/vkext-flex.h"
#include "vkext/vk_zend.h"

#include "flex/flex.h"

#define BUFF_LEN (1 << 16)
static char buff[BUFF_LEN];
extern int verbosity;

char *do_flex(const char *name, size_t name_len, const char *case_name, size_t case_name_len, bool sex, const char *type, size_t type_len, int lang_id) {
  const size_t error_msg_buf_size = 1000;
  static char ERROR_MSG_BUF[error_msg_buf_size] = {'\0'};
  ERROR_MSG_BUF[0] = '\0';
  vk::string_view res = flex(vk::string_view{name, name_len}, vk::string_view{case_name, case_name_len}, sex, vk::string_view{type, type_len}, lang_id, buff,
                             ERROR_MSG_BUF, error_msg_buf_size);
  if (verbosity && ERROR_MSG_BUF[0] != '\0') {
    fprintf(stderr, "%s\n", ERROR_MSG_BUF);
  }
  return estrndup(res.data(), res.size());
}
