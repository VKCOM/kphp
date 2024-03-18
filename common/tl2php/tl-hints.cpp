// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl2php/tl-hints.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>

#include "common/tlo-parsing/tlo-parsing.h"

namespace vk {
namespace tl {

void trim(std::string &str) {
  str.erase(str.begin(), std::find_if_not(str.begin(), str.end(), isspace));
  str.erase(std::find_if_not(str.rbegin(), str.rend(), isspace).base(), str.end());
}

struct FileLineReader {
  explicit FileLineReader(const std::string &combined2_tl_file)
    : combined2_tl_file_(combined2_tl_file)
    , combined2_tl_(combined2_tl_file_) {
    if (!combined2_tl_) {
      raise_parsing_excpetion(std::strerror(errno));
    }
  }

  void raise_parsing_excpetion(const std::string &what) {
    std::string line_hint;
    if (line_number_) {
      line_hint = ":" + std::to_string(line_number_);
    }
    throw std::runtime_error{"Error on reading tl hints from '" + combined2_tl_file_ + line_hint + "': " + std::string{what}};
  }

  void check_pos(size_t pos, const char *what) {
    if (pos == std::string::npos || pos + 1 >= line_.size()) {
      raise_parsing_excpetion(std::string{what} + " is expected");
    }
  }

  bool read_next_line() {
    line_.clear();
    while (combined2_tl_) {
      ++line_number_;
      std::getline(combined2_tl_, line_);
      if (!line_.empty()) {
        break;
      }
    }
    return !line_.empty();
  }

  const std::string &get_current_line() const {
    return line_;
  }

private:
  const std::string combined2_tl_file_;
  std::ifstream combined2_tl_;
  std::string line_;
  size_t line_number_{0};
};

void TlHints::load_from_combined2_tl_file(const std::string &combined2_tl_file, bool rename_all_forbidden_names) {
  FileLineReader reader{combined2_tl_file};

  while (reader.read_next_line()) {
    const auto &line = reader.get_current_line();

    size_t name_end_pos = line.find('#');
    reader.check_pos(name_end_pos, "name");
    std::string name = line.substr(0, name_end_pos);
    trim(name);
    if (rename_all_forbidden_names) {
      tlo_parsing::rename_tl_name_if_forbidden(name);
    }

    size_t magic_end_pos = line.find(' ', name_end_pos + 1);
    reader.check_pos(magic_end_pos, "magic");
    std::string magic = line.substr(name_end_pos + 1, magic_end_pos - name_end_pos - 1);
    trim(magic);

    size_t result_pos = line.rfind('=');
    reader.check_pos(result_pos, "result");
    std::string result = line.substr(result_pos + 1);
    result = result.substr(0, result.find("//"));
    trim(result);

    std::vector<std::string> args;
    size_t arg_end_pos = result_pos - 1;
    while (true) {
      assert(arg_end_pos > 1);
      size_t search_colon_from = arg_end_pos - 1;
      if (line.at(search_colon_from) == ']') {
        const size_t new_search_colon_from = line.rfind('[', search_colon_from);
        if (line.find(']', new_search_colon_from) != search_colon_from) {
          reader.raise_parsing_excpetion("anonymous array combinations are forbidden");
        }
        search_colon_from = new_search_colon_from;
      }

      size_t colon_pos = line.rfind(':', search_colon_from);
      if (colon_pos == std::string::npos) {
        break;
      }

      reader.check_pos(colon_pos, "arg colon");
      size_t arg_start_pos = line.rfind(' ', colon_pos);
      reader.check_pos(arg_start_pos, "arg");

      auto arg = line.substr(arg_start_pos + 1, arg_end_pos - arg_start_pos - 1);
      trim(arg);
      args.emplace_back(std::move(arg));
      arg_end_pos = arg_start_pos;
    }

    std::reverse(args.begin(), args.end());
    auto it = hints_.emplace(std::move(name), TlHint{std::move(magic), std::move(args), std::move(result)});
    if (!it.second) {
      reader.raise_parsing_excpetion("it is forbidden to override combinator");
    }
  }
}

const TlHint *TlHints::get_hint_for_combinator(const std::string &combinator_name) const {
  auto it = hints_.find(combinator_name);
  return it == hints_.end() ? nullptr : &it->second;
}

} // namespace tl
} // namespace vk
