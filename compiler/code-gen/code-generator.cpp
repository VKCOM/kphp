// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/code-generator.h"


void CodeGenerator::open_file_create_writer(bool compile_with_debug_info_flag, bool compile_with_crc, const std::string &file_name, const std::string &subdir) {
  kphp_assert(data == nullptr);
  data = new WriterData(compile_with_debug_info_flag, compile_with_crc, file_name, subdir);
  data->begin_line();

  // restore to initial values in case the previous file had spoilt them
  indent_level = 0;
  need_indent = false;
  lock_comments_cnt = 1;
}

void CodeGenerator::close_file_clear_writer() {
  kphp_assert(data != nullptr);
  data->end_line();
  data->calc_crc();
  os << data;
  data = nullptr;   // do not delete data, as it will be used by and deleted by the next pipe
}
