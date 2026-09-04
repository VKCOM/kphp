// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/src-file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "auto/compiler/vertex/vertex-all.h"
#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
#include "compiler/data/lib-data.h"
#include "compiler/stage.h"

bool SrcFile::load() {
  if (loaded) {
    return true;
  }
  int err;

  int fid = open(file_name.c_str(), O_RDONLY);
  kphp_assert_msg(fid >= 0, fmt_format("failed to open file [{}] : {}", file_name, strerror(errno)));

  struct stat buf;
  err = fstat(fid, &buf);
  kphp_assert_msg(err >= 0, "fstat failed : %m");

  kphp_assert_msg(buf.st_size < 100000000, fmt_format("file [{}] is too big [{}]\n", file_name, buf.st_size));
  int file_size = (int)buf.st_size;
  text = std::string(file_size, ' ');
  err = (int)read(fid, &text[0], file_size);
  kphp_assert_msg(err >= 0, fmt_format("Can't read file [{}]: {}", file_name, strerror(errno)));

  for (int i = 0, prev_i = 0; i < file_size; i++) {
    if (unlikely(text[i] == 0)) {
      kphp_warning(fmt_format("symbol with code zero was replaced by space in file [{}] at [{}]", file_name, i));
      text[i] = ' ';
    }
    if (text[i] == '\n') {
      lines.push_back(vk::string_view(&text[prev_i], &text[i]));
      prev_i = i + 1;
    }
  }

  loaded = true;
  close(fid);

  return true;
}

vk::string_view SrcFile::get_line(int id) {
  id--;
  if (id < 0 || id >= (int)lines.size()) {
    return {};
  }
  return lines[id];
}

bool SrcFile::is_from_owner_lib() const {
  return owner_lib && !owner_lib->is_raw_php() && file_name == owner_lib->functions_txt_file();
}

std::string SrcFile::get_main_func_run_var_name() const {
  return main_func_name + "$called";
}

VertexPtr SrcFile::get_main_func_run_var() const {
  auto v = VertexAdaptor<op_var>::create();
  v->set_string(get_main_func_run_var_name());
  v->extra_type = op_ex_var_superglobal;
  return v;
}
