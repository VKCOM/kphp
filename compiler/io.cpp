#include "compiler/io.h"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/crc32.h"

#include "compiler/compiler-core.h"
#include "compiler/data/src-file.h"
#include "compiler/stage.h"

Writer::Writer() :
  state(w_stopped),
  data(),
  callback(nullptr),
  indent_level(0),
  need_indent(false),
  lock_comments_cnt(1) {
}

void Writer::write_indent() {
  need_indent = false;
  data.append(indent_level, ' ');
}

inline void Writer::begin_line() {
  data.begin_line();
}

inline void Writer::end_line() {
  data.end_line();

  data.append(1, '\n'); // for crc64
  need_indent = true;
}

void Writer::set_file_name(const string &file_name, const string &subdir) {
  data.file_name = file_name;
  data.subdir = subdir;
}

void Writer::set_callback(WriterCallbackBase *new_callback) {
  callback = new_callback;
}

void Writer::begin_write(bool compile_with_debug_info_flag, bool compile_with_crc) {
  assert (state == w_stopped);
  state = w_running;

  indent_level = 0;
  need_indent = false;
  data = WriterData(compile_with_debug_info_flag, compile_with_crc);
  begin_line();
}

void Writer::end_write() {
  end_line();

  if (callback != nullptr) {
    callback->on_end_write(std::move(data));
  }

  assert (state == w_running);
  state = w_stopped;
}

void Writer::indent(int diff) {
  indent_level += diff;
}

void Writer::new_line() {
  end_line();
  begin_line();
}

void Writer::brk() {
  data.brk();
}

void Writer::add_location(SrcFilePtr file, int line_num) {
  if (lock_comments_cnt > 0) {
    return;
  }
//  fprintf (stdout, "In %s:%d\n", file->short_file_name.c_str(), line_num);
  data.add_location(file, line_num);
}

void Writer::lock_comments() {
  lock_comments_cnt++;
  brk();
}

void Writer::unlock_comments() {
  lock_comments_cnt--;
}

bool Writer::is_comments_locked() {
  return lock_comments_cnt > 0;
}

void Writer::add_include(const string &s) {
  data.add_include(s);
}

void Writer::add_lib_include(const string &s) {
  data.add_lib_include(s);
}

