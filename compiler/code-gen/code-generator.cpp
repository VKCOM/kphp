// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/compiler-core.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/data/src-file.h"

void CodeGenerator::open_file_create_writer(bool compile_with_debug_info_flag, bool compile_with_crc, const std::string &file_name, const std::string &subdir) {
  std::string full_file_name = G->cpp_dir;
  if (!subdir.empty()) {
    full_file_name += subdir;
    full_file_name += "/";
  }
  full_file_name += file_name;

  const auto &exclude_from_debug = G->get_exclude_namespaces();
  for (const auto &exclude_symbol : exclude_from_debug) {
    if (file_name.rfind(exclude_symbol, 0) != std::string::npos) {
      compile_with_debug_info_flag = false;
    }
  }

  // get a currently opened file from a cpp index saved at previous kphp launch — to compare hashes with it
  // (if it doesn't exist in index, its hashes will be -1; if !compile_with_crc, the same)
  cur_file = G->get_file_info(std::move(full_file_name));
  cur_file->compile_with_debug_info_flag = compile_with_debug_info_flag;
  cur_file->needed = true;
  // also, cur_file->includes will be modified on demand, see add_include()

  if (is_step_just_calc_hashes) {

  } else {
    kphp_assert(data == nullptr);
    data = new WriterData(cur_file, compile_with_crc);
    data->begin_line();
  }

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

  if (is_step_just_calc_hashes) {
    kphp_assert(cur_file != nullptr);

    bool cur_hashes_diff = cur_file->crc64 != hash_of_cpp || cur_file->crc64_with_comments != hash_of_comments;
    if (cur_hashes_diff) {
      diff_files_count++;
      // in this case, the same codegen command will be passed to the next pipe, see CodeGenForDiffF
    }
    cur_file = nullptr;

  } else {
    kphp_assert(data != nullptr);
    data->end_line();
    data->set_calculated_hashes(hash_of_cpp, hash_of_comments);
    os << data;       // pass it to WriteFilesF
    data = nullptr;   // do not delete data, as it will be used by and deleted by WriteFilesF
  }
}

void CodeGenerator::feed_hash_of_comments(SrcFilePtr file, int line_num) {
  vk::string_view line_contents = file->get_line(line_num);
  hash_of_comments = hash_of_comments * 56235415617457ULL + string_hash(line_contents.data(), line_contents.size());
}

void CodeGenerator::add_include(const std::string &s) {
  kphp_assert(cur_file != nullptr);
  feed_hash(string_hash(s.c_str(), s.size()));
  // we need to store includes even when just calculating hashes — to make a dependency map for make
  cur_file->includes.emplace_front(s);
}

void CodeGenerator::add_lib_include(const std::string &s) {
  kphp_assert(cur_file != nullptr);
  feed_hash(string_hash(s.c_str(), s.size()));
  cur_file->lib_includes.emplace_front(s);
}
