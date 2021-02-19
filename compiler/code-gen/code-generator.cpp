// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/src-file.h"


void CodeGenerator::open_file_create_writer(bool compile_with_debug_info_flag, bool compile_with_crc, const std::string &file_name, const std::string &subdir) {
  kphp_assert(data == nullptr);
  data = new WriterData(compile_with_debug_info_flag, compile_with_crc, file_name, subdir);
  data->begin_line();

  // start calculating hashes of the newly opened file
  hash_of_cpp = 0;
  hash_of_comments = 0;

  // restore to initial values in case the previous file had spoilt them
  indent_level = 0;
  need_indent = false;
  lock_comments_cnt = 1;
}

void CodeGenerator::close_file_clear_writer() {
  // don't save -1 as a hash of the file, as -1 is a reserved value meaning "not initialized"
  if (hash_of_cpp == static_cast<unsigned long long>(-1)) {
    hash_of_cpp = 463721894672819432ULL;
  }

  kphp_assert(data != nullptr);
  data->end_line();
  data->set_calculated_hashes(hash_of_cpp, hash_of_comments);
  os << data;
  data = nullptr;   // do not delete data, as it will be used by and deleted by the next pipe
}

void CodeGenerator::feed_hash_of_comments(SrcFilePtr file, int line_num) {
  vk::string_view line_contents = file->get_line(line_num);
  hash_of_comments = hash_of_comments * 56235515617499ULL + string_hash(line_contents.data(), line_contents.size());
}
