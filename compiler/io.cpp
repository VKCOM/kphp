#include "compiler/io.h"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/crc32.h"

#include "compiler/stage.h"

SrcFile::SrcFile()
  : id(0)
  , loaded(false)
  , is_required(false)
{}

SrcFile::SrcFile (const string &file_name, const string &short_file_name, const string &class_context)
  : id(0)
  , file_name(file_name)
  , short_file_name(short_file_name)
  , loaded(false)
  , is_required(false)
  , class_context(class_context)
{}

void SrcFile::add_prefix (const string &new_prefix) {
  prefix = new_prefix;
}

bool SrcFile::load(void) {
  if (loaded) {
    return true;
  }
  int err;

  int fid = open (file_name.c_str(), O_RDONLY);
  dl_passert (fid >= 0, dl_pstr ("failed to open file [%s]", file_name.c_str()));

  struct stat buf;
  err = fstat (fid, &buf);
  dl_passert (err >= 0, "fstat failed");

  dl_assert (buf.st_size < 100000000, dl_pstr ("file [%s] is too big [%lu]\n", file_name.c_str(), buf.st_size));
  int file_size = (int)buf.st_size;
  int prefix_size = (int)prefix.size();
  int text_size = file_size + prefix_size;
  text = string (text_size, ' ');
  std::copy (prefix.begin(), prefix.end(), text.begin());
  err = (int)read (fid, &text[0] + prefix_size, file_size);
  dl_assert (err >= 0, dl_pstr("Can't read file [%s]: %m", file_name.c_str()));

  for (int i = 0; i < text_size; i++) {
    if (likely (text[i] == 0)) {
      kphp_warning (dl_pstr ("symbol with code zero was replaced by space in file [%s] at [%d]", file_name.c_str(), i - prefix_size));
      text[i] = ' ';
    }
  }

  for (int i = prefix_size, prev_i = prefix_size; i <= text_size; i++) {
    if (text[i] == '\n') {
      lines.push_back (string_ref (&text[prev_i], &text[i]));
      prev_i = i + 1;
    }
  }

  loaded = true;
  close (fid);

  return true;
}

string_ref SrcFile::get_line (int id) {
  id--;
  if (id < 0 || id >= (int)lines.size()) {
    return string_ref();
  }
  return lines[id];
}

WriterData::WriterData(bool compile_with_debug_info_flag) :
  lines(),
  text(),
  crc (-1),
  compile_with_debug_info_flag(compile_with_debug_info_flag),
  file_name(),
  subdir() {
}


void WriterData::append (const char *begin, size_t length) {
//  fprintf (stdout, "%*s\n", (int)length, begin);
  text.append (begin, length);
}
void WriterData::append (size_t n, char c) {
  text.append (n, c);
}
void WriterData::begin_line() {
//  fprintf (stdout, "<<<<BEGIN>>>>\n");
  lines.push_back (Line ((int)text.size()));
}
void WriterData::end_line() {
//  fprintf (stdout, "<<<< END >>>>\n");
  lines.back().end_pos = (int)text.size();
}
void WriterData::brk() {
  lines.back().brk = true;
}
void WriterData::add_location (SrcFilePtr file, int line) {
  if (file.is_null()) {
    return;
  }
  if (lines.back().file.not_null() && !(lines.back().file == file)) {
    return;
  }
  kphp_error (
      lines.back().file.is_null() || lines.back().file == file,
      dl_pstr ("%s|%s", file->file_name.c_str(), lines.back().file->file_name.c_str())
  );
  lines.back().file = file;
  if (line != -1) {
    lines.back().line_ids.insert (line);
  }
}
void WriterData::add_include (const string &s) {
  includes.push_back (s);
}
const vector <string> &WriterData::get_includes() {
  return includes;
}
unsigned long long WriterData::calc_crc() {
  if (crc == (unsigned long long)-1) {
    crc = compute_crc64 (text.c_str(), (int)text.length());
    if (crc == (unsigned long long)-1) {
      crc = 463721894672819432ull;
    }
  }
  return crc;
}

void WriterData::write_code (string &dest_str, const Line &line) {
  const char *s = &text[line.begin_pos];
  int length = line.end_pos - line.begin_pos;
  dest_str.append(s, length);
  dest_str += "\n";
}

template <class T> void WriterData::dump (string &dest_str, T begin, T end, SrcFilePtr file) {
  int l = (int)1e9, r = -1;

  if (file.not_null()) {
    dest_str += "//source = [";
    dest_str += file->unified_file_name.c_str();
    dest_str += "]\n";
  }

  vector <int> rev;
  for (int t = 0; t < 3; t++) {
    int pos = 0, end_pos = 0, cur_id = l - 1;

    T cur_line = begin;
    for (T i = begin; i != end; i++, pos++) {
      for (__typeof (i->line_ids.begin()) line = i->line_ids.begin(); line != i->line_ids.end(); line++) {
        int id = *line;
        if (t == 0) {
          if (id < l) {
            l = id;
          }
          if (r < id) {
            r = id;
          }
        } else if (t == 1) {
          if (rev[id - l] < pos) {
            rev[id - l] = pos;
          }
        } else {
          while (cur_id < id) {
            cur_id++;
            if (cur_id + 10 > id) {
              dest_str += dl_pstr ("//%d: ", cur_id);
              string_ref comment = file->get_line (cur_id);
              int last_printed = ':';
              for (int j = 0, nj = comment.length(); j < nj; j++) {
                int c = comment.begin()[j];
                if (c == '\n') {
                  dest_str += "\\n";
                  last_printed = 'n';
                } else if (c > 13) {
                  dest_str += c;
                  if (c > 32) {
                    last_printed = c;
                  }
                }
              }
              if (last_printed == '\\') {
                dest_str += ";";
              }
              dest_str += "\n";
            }

            int new_pos = rev[cur_id - l];
            if (end_pos < new_pos) {
              end_pos = new_pos;
            }
          }
        }
      }

      if (t == 2) {
        if (pos == end_pos) {
          while (cur_line != i) {
            write_code (dest_str, *cur_line);
            cur_line++;
          }
          do {
            write_code (dest_str, *cur_line);
            cur_line++;
          } while (cur_line != end && cur_line->line_ids.empty());
        }
      }
    }


    if (t == 0) {
      if (r == -1) {
        l = -1;
      }
      //fprintf (stderr, "l = %d, r = %d\n", l, r);
      assert (l <= r);
      rev.resize (r - l + 1, -1);
    } else if (t == 2) {
      while (cur_line != end) {
        write_code (dest_str, *cur_line);
        cur_line++;
      }
    }
  }

}

void WriterData::dump (string &dest_str) {
  for (__typeof (lines.begin()) i = lines.begin(); i != lines.end();) {
    if (i->file.is_null()) {
      dump (dest_str, i, i + 1, SrcFilePtr());
      i++;
      continue;
    }

    __typeof (lines.begin()) j;
    for (j = i + 1; j != lines.end() && (j->file.is_null() || i->file == j->file) && !j->brk; j++) {
    }
    dump (dest_str, i, j, i->file);
    i = j;
  }
}

void WriterData::swap (WriterData &other) {
  std::swap (lines, other.lines);
  std::swap (text, other.text);
  std::swap (crc, other.crc);
  std::swap (file_name, other.file_name);
  std::swap (subdir, other.subdir);
  std::swap (includes, other.includes);
  std::swap (compile_with_debug_info_flag, other.compile_with_debug_info_flag);
}

bool WriterData::compile_with_debug_info() const {
  return compile_with_debug_info_flag;
}

Writer::Writer() :
  state (w_stopped),
  data(),
  callback (NULL),
  indent_level (0),
  need_indent (0),
  lock_comments_cnt (1) {
}

Writer::~Writer() {
}

void Writer::write_indent(void) {
  append (indent_level, ' ');
}

void Writer::append (const char *begin, size_t length) {
  data.append (begin, length);
}
void Writer::append (size_t n, char c) {
  data.append (n, c);
}
void Writer::begin_line (void) {
  data.begin_line();
}
void Writer::end_line (void) {
  data.end_line();

  data.append (1, '\n'); // for crc64
  need_indent = 1;
}

void Writer::set_file_name (const string &file_name, const string &subdir) {
  data.file_name = file_name;
  data.subdir = subdir;
}
void Writer::set_callback (WriterCallbackBase *new_callback) {
  callback = new_callback;
}

void Writer::begin_write(bool compile_with_debug_info_flag) {
  assert (state == w_stopped);
  state = w_running;

  indent_level = 0;
  need_indent = 0;
  data = WriterData(compile_with_debug_info_flag);
  begin_line();
}
void Writer::end_write() {
  end_line();

  if (callback != NULL) {
    callback->on_end_write (&data);
  }

  assert (state == w_running);
  state = w_stopped;
}

void Writer::operator() (const string &s) {
  if (need_indent) {
    need_indent = 0;
    write_indent();
  }
  append (s.c_str(), s.size());
}
void Writer::operator() (const char *s) {
  if (need_indent) {
    need_indent = 0;
    write_indent();
  }
  append (s, strlen (s));
}
void Writer::operator() (const string_ref &s) {
  if (need_indent) {
    need_indent = 0;
    write_indent();
  }
  append (s.begin(), s.length());
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

void Writer::operator() (SrcFilePtr file, int line_num) {
  if (lock_comments_cnt > 0) {
    return;
  }
//  fprintf (stdout, "In %s:%d\n", file->short_file_name.c_str(), line_num);
  data.add_location (file, line_num);
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
void Writer::add_include (const string &s) {
  data.add_include (s);
}


