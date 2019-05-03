#include "compiler/code-gen/writer-data.h"

#include "common/crc32.h"

#include "compiler/data/src-file.h"
#include "compiler/stage.h"

WriterData::WriterData(bool compile_with_debug_info_flag, bool compile_with_crc) :
  lines(),
  text(),
  crc(-1),
  compile_with_debug_info_flag(compile_with_debug_info_flag),
  compile_with_crc_flag(compile_with_crc),
  file_name(),
  subdir() {
}


void WriterData::begin_line() {
  lines.emplace_back((int)text.size());
}

void WriterData::end_line() {
  lines.back().end_pos = (int)text.size();
}

void WriterData::brk() {
  lines.back().brk = true;
}

void WriterData::add_location(SrcFilePtr file, int line) {
  if (!file) {
    return;
  }
  if (lines.back().file && !(lines.back().file == file)) {
    return;
  }
  kphp_error (
    !lines.back().file || lines.back().file == file,
    format("%s|%s", file->file_name.c_str(), lines.back().file->file_name.c_str())
  );
  lines.back().file = file;
  if (line != -1) {
    lines.back().line_ids.insert(line);
  }
}

void WriterData::add_include(const string &s) {
  includes.push_back(s);
}

const vector<string> &WriterData::get_includes() const {
  return includes;
}

void WriterData::add_lib_include(const string &s) {
  lib_includes.push_back(s);
}

const vector<string> &WriterData::get_lib_includes() const {
  return lib_includes;
}

unsigned long long WriterData::calc_crc() {
  if (crc == (unsigned long long)-1) {
    crc = compute_crc64(text.c_str(), (int)text.length());
    if (crc == (unsigned long long)-1) {
      crc = 463721894672819432ull;
    }
  }
  return crc;
}

void WriterData::write_code(string &dest_str, const Line &line) {
  const char *s = &text[line.begin_pos];
  int length = line.end_pos - line.begin_pos;
  dest_str.append(s, length);
  dest_str += '\n';
}

template<class T>
void WriterData::dump(string &dest_str, const T &begin, const T &end, SrcFilePtr file) {
  int l = (int)1e9, r = -1;

  if (file) {
    dest_str += "//source = [";
    dest_str += file->unified_file_name;
    dest_str += "]\n";
  }

  vector<int> rev;
  for (int t = 0; t < 3; t++) {
    int pos = 0, end_pos = 0, cur_id = l - 1;

    T cur_line = begin;
    for (T i = begin; i != end; i++, pos++) {
      for (int id : i->line_ids) {
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
              dest_str += format("//%d: ", cur_id);
              vk::string_view comment = file->get_line(cur_id);
              char last_printed = ':';
              for (char c : comment) {
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
                dest_str += ';';
              }
              dest_str += '\n';
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
            write_code(dest_str, *cur_line);
            cur_line++;
          }
          do {
            write_code(dest_str, *cur_line);
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
      rev.resize(r - l + 1, -1);
    } else if (t == 2) {
      while (cur_line != end) {
        write_code(dest_str, *cur_line);
        cur_line++;
      }
    }
  }

}

void WriterData::dump(string &dest_str) {
  for (auto i = lines.begin(); i != lines.end();) {
    if (!i->file) {
      dump(dest_str, i, i + 1, SrcFilePtr());
      i++;
      continue;
    }

    decltype(i) j;
    for (j = i + 1; j != lines.end() && (!j->file || i->file == j->file) && !j->brk; j++) {
    }
    dump(dest_str, i, j, i->file);
    i = j;
  }
}

void WriterData::swap(WriterData &other) {
  std::swap(lines, other.lines);
  std::swap(text, other.text);
  std::swap(crc, other.crc);
  std::swap(file_name, other.file_name);
  std::swap(subdir, other.subdir);
  std::swap(includes, other.includes);
  std::swap(lib_includes, other.lib_includes);
  std::swap(compile_with_debug_info_flag, other.compile_with_debug_info_flag);
  std::swap(compile_with_crc_flag, other.compile_with_crc_flag);
}

bool WriterData::compile_with_debug_info() const {
  return compile_with_debug_info_flag;
}

bool WriterData::compile_with_crc() const {
  return compile_with_crc_flag;
}
