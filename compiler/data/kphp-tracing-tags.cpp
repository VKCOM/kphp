// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/kphp-tracing-tags.h"

#include "common/algorithms/string-algorithms.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"

int KphpTracingDeclarationMixin::parse_level_attr(vk::string_view beg, size_t &pos_end) {
  int level = 0;
  int len = beg.size();

  for (; pos_end < len && isspace(beg[pos_end]); ++pos_end) {}
  for (; pos_end < len && isdigit(beg[pos_end]); ++pos_end) {
    level = level * 10 + (beg[pos_end] - '0');
  }
  kphp_error_act(level >= 1 && level <= 2,
                 "invalid 'level' in @kphp-tracing: expected to be 1/2", return 1);

  return level;
}

std::string KphpTracingDeclarationMixin::parse_string_attr(vk::string_view beg, size_t &pos_end) {
  std::string out;
  size_t len = beg.size();

  for (; pos_end < len && isspace(beg[pos_end]); ++pos_end) {}

  char quote = '\0';
  if (pos_end < len && (beg[pos_end] == '"' || beg[pos_end] == '\'')) {
    quote = beg[pos_end];
    pos_end++;
  }

  for (; pos_end < len; ++pos_end) {  // eat a string until it ends
    char c = beg[pos_end];
    if (isspace(c) && quote == '\0') {
      break;
    }
    if (quote != '\0' && c == quote) {
      pos_end++;
      break;
    }
    if (c == '\\' && pos_end < len + 1) {
      out += beg[++pos_end];
      continue;
    }
    out += c;
  }

  kphp_error(!out.empty() || quote != '\0', "empty string in @kphp-tracing");
  return out;
}

std::pair<int, std::string> KphpTracingDeclarationMixin::parse_branch_attr(vk::string_view beg, size_t &pos_end) {
  int branch_num = 0;
  size_t len = beg.size();

  for (; pos_end < len && isspace(beg[pos_end]); ++pos_end) {}
  for (; pos_end < len && isdigit(beg[pos_end]); ++pos_end) {
    branch_num = branch_num * 10 + (beg[pos_end] - '0');
  }
  kphp_error_act(branch_num != 0 && pos_end < len && beg[pos_end] == ':',
                 "invalid 'branch' format in @kphp-tracing", return {});
  kphp_error_act(branch_num >= 1 && branch_num <= 3,
                 "invalid 'branch' in @kphp-tracing: number can be 1/2/3 only", return {});
  pos_end++;

  size_t pos_end_str = 0;
  std::string cond_title = parse_string_attr(beg.substr(pos_end), pos_end_str);
  pos_end += pos_end_str;

  return {branch_num, cond_title};
}

std::string KphpTracingDeclarationMixin::generate_default_span_title(FunctionPtr f) {
  std::string title;
  if (f->is_lambda()) {
    title = "lambda in ";
    while (f->is_lambda()) {
      f = f->outer_function;
    }
  }

  if (!f->class_id) {
    return title + f->name;
  }

  vk::string_view class_name = f->context_class ? f->context_class->name : f->class_id->name;
  if (size_t rpos = class_name.rfind("\\"); rpos != std::string::npos) {
    class_name = class_name.substr(rpos + 1);
  }
  title += static_cast<std::string>(class_name) + "::" + f->local_name();
  if (size_t rpos = title.rfind("$$"); rpos != std::string::npos) {
    title = title.substr(0, rpos);  // drop context class
  }
  return title;
}

KphpTracingDeclarationMixin *KphpTracingDeclarationMixin::create_for_function_from_phpdoc(FunctionPtr f, vk::string_view value) {
  KphpTracingDeclarationMixin *m = new KphpTracingDeclarationMixin();

  while (true) {
    value = vk::ltrim(value);
    if (value.empty() || stage::has_error()) {
      break;
    }

    size_t pos_eq = value.find('=');
    if (pos_eq == std::string::npos) {
      kphp_error(0, fmt_format("Unknown '{}' in @kphp-tracing", value));
      break;
    }

    size_t pos_end = 0;
    vk::string_view attr_name = vk::rtrim(value.substr(0, pos_eq));
    if (attr_name == "title") {
      m->span_title = parse_string_attr(value.substr(pos_eq + 1), pos_end);
    } else if (attr_name == "level") {
      m->level = parse_level_attr(value.substr(pos_eq + 1), pos_end);
    } else if (attr_name == "aggregate") {
      m->aggregate_name = parse_string_attr(value.substr(pos_eq + 1), pos_end);
    } else if (attr_name == "branch") {
      kphp_error(m->is_aggregate(), "'branch' is applicable only if 'aggregate' is set in @kphp-tracing");
      m->branches.emplace_front(parse_branch_attr(value.substr(pos_eq + 1), pos_end));
    } else {
      kphp_error(0, fmt_format("Unknown '{}' in @kphp-tracing", attr_name));
      break;
    }

    value = value.substr(pos_eq + 1 + pos_end);
  }

  if (m->span_title.empty()) {
    m->span_title = generate_default_span_title(f);
  }

  return m;
}

KphpTracingDeclarationMixin *KphpTracingDeclarationMixin::create_for_shutdown_function(FunctionPtr f) {
  KphpTracingDeclarationMixin *m = new KphpTracingDeclarationMixin();

  m->span_title = generate_default_span_title(f);
  
  return m;
}
