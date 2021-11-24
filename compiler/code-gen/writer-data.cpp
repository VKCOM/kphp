// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/writer-data.h"

#include "compiler/data/src-file.h"
#include "compiler/stage.h"
#include "common/wrappers/fmt_format.h"

WriterData::WriterData(File *file, bool compile_with_crc) :
  file(file),
  lines(),
  text(),
  hash_of_cpp(-1),
  hash_of_comments(-1),
  compile_with_crc_flag(compile_with_crc) {
}

void WriterData::begin_line() {
  lines.emplace_back((int)text.size());
}

void WriterData::end_line() {
  lines.back().end_pos = (int)text.size();
  append('\n');
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
    fmt_format("{}|{}", file->file_name, lines.back().file->file_name)
  );
  lines.back().file = file;
  if (line != -1) {
    lines.back().line_ids.insert(line);
  }
}

void WriterData::set_calculated_hashes(unsigned long long int hash_of_cpp, unsigned long long int hash_of_comments) {
  this->hash_of_cpp = hash_of_cpp;
  this->hash_of_comments = hash_of_comments;
}

void WriterData::write_code(std::string &dest_str, const Line &line) {
  const char *s = &text[line.begin_pos];
  int length = line.end_pos - line.begin_pos;
  dest_str.append(s, length);
  dest_str += '\n';
}

void WriterData::dump(std::string &dest_str, const std::vector<Line>::iterator &begin, const std::vector<Line>::iterator &end, SrcFilePtr file) {
  int l = (int)1e9, r = -1;

  if (file) {
    dest_str += "//source = [";
    dest_str += file->relative_file_name;
    dest_str += "]\n";
  }

  std::vector<int> rev;
  for (int t = 0; t < 3; t++) {
    int pos = 0, end_pos = 0, cur_id = l - 1;

    auto cur_line = begin;
    for (auto i = begin; i != end; i++, pos++) {
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
              dest_str += fmt_format("//{}: ", cur_id);
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

void WriterData::dump(std::string &dest_str) {
  for (auto i = lines.begin(); i != lines.end();) {
    if (!i->file) {
      dump(dest_str, i, i + 1, SrcFilePtr());
      i++;
      continue;
    }

    auto j = std::find_if(std::next(i), lines.end(),
                          [i](const Line &l) {
                            return (l.file && i->file != l.file) || l.brk;
                          });
    dump(dest_str, i, j, i->file);
    i = j;
  }
}
