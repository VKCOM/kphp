#include "compiler/data/src-file.h"

#include <fcntl.h>

#include "compiler/stage.h"

SrcFile::SrcFile() :
  id(0),
  loaded(false),
  is_required(false) {}

SrcFile::SrcFile(const string &file_name, const string &short_file_name, const string &class_context) :
  id(0),
  file_name(file_name),
  short_file_name(short_file_name),
  loaded(false),
  is_required(false),
  class_context(class_context) {}

void SrcFile::add_prefix(const string &new_prefix) {
  prefix = new_prefix;
}

bool SrcFile::load() {
  if (loaded) {
    return true;
  }
  int err;

  int fid = open(file_name.c_str(), O_RDONLY);
  dl_passert (fid >= 0, dl_pstr("failed to open file [%s]", file_name.c_str()));

  struct stat buf;
  err = fstat(fid, &buf);
  dl_passert (err >= 0, "fstat failed");

  dl_assert (buf.st_size < 100000000, dl_pstr("file [%s] is too big [%lu]\n", file_name.c_str(), buf.st_size));
  int file_size = (int)buf.st_size;
  int prefix_size = (int)prefix.size();
  int text_size = file_size + prefix_size;
  text = string(text_size, ' ');
  std::copy(prefix.begin(), prefix.end(), text.begin());
  err = (int)read(fid, &text[0] + prefix_size, file_size);
  dl_assert (err >= 0, dl_pstr("Can't read file [%s]: %m", file_name.c_str()));

  for (int i = 0; i < text_size; i++) {
    if (unlikely (text[i] == 0)) {
      kphp_warning(dl_pstr("symbol with code zero was replaced by space in file [%s] at [%d]", file_name.c_str(), i - prefix_size));
      text[i] = ' ';
    }
  }

  for (int i = prefix_size, prev_i = prefix_size; i <= text_size; i++) {
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

