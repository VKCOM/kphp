#include "compiler/data/src-file.h"

#include <fcntl.h>

#include "compiler/compiler-core.h"
#include "compiler/stage.h"

SrcFile::SrcFile() :
  id(0),
  loaded(false),
  is_required(false) {}

SrcFile::SrcFile(const string &file_name, const string &short_file_name, LibPtr owner_lib_id) :
  id(0),
  file_name(file_name),
  short_file_name(short_file_name),
  loaded(false),
  is_required(false),
  owner_lib(owner_lib_id) {}


bool SrcFile::load() {
  if (loaded) {
    return true;
  }
  int err;

  int fid = open(file_name.c_str(), O_RDONLY);
  dl_passert (fid >= 0, format("failed to open file [%s]", file_name.c_str()));

  struct stat buf;
  err = fstat(fid, &buf);
  dl_passert (err >= 0, "fstat failed");

  dl_assert (buf.st_size < 100000000, format("file [%s] is too big [%lu]\n", file_name.c_str(), buf.st_size));
  int file_size = (int)buf.st_size;
  text = string(file_size, ' ');
  err = (int)read(fid, &text[0], file_size);
  dl_assert (err >= 0, format("Can't read file [%s]: %m", file_name.c_str()));

  for (int i = 0; i < file_size; i++) {
    if (unlikely (text[i] == 0)) {
      kphp_warning(format("symbol with code zero was replaced by space in file [%s] at [%d]", file_name.c_str(), i));
      text[i] = ' ';
    }
  }

  for (int i = 0, prev_i = 0; i <= file_size; i++) {
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
    return vk::string_view(nullptr, nullptr);
  }
  return lines[id];
}

string SrcFile::get_short_name() {
  string root_path = G->env().get_base_dir();
  if (vk::string_view(file_name).starts_with(root_path)) {
    return file_name.substr(root_path.length());
  }
  return file_name;
}
