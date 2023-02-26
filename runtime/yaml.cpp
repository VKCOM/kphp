#include <yaml-cpp/yaml.h>

#include "runtime/optional.h"
#include "runtime/streams.h"
#include "runtime/critical_section.h"
#include "runtime/yaml.h"

void yaml_node_to_mixed(const YAML::Node &node, mixed &data, const string &source) {
  data.clear(); // sets data to NULL
  if (node.IsScalar()) {
    const string string_data(node.as<std::string>().c_str());
    if (string_data == string("true") || string_data == string("false")) {
      if (source[node.Mark().pos] == '"' && source[node.Mark().pos + string_data.size() + 1] == '"') {
        data = string_data;
      } else {
        data = string_data == string("true");
      }
    } else if (string_data.is_int()) {
      if (source[node.Mark().pos] == '"' && source[node.Mark().pos + string_data.size() + 1] == '"') {
        data = string_data;
      } else {
        data = string_data.to_int();
      }
    } else {
      dl::enter_critical_section();
      auto *float_data = new double;
      if (string_data.try_to_float(float_data)) {
        if (source[node.Mark().pos] == '"' && source[node.Mark().pos + string_data.size() + 1] == '"') {
          data = string_data;
        } else {
          data = *float_data;
        }
      } else {
        data = string_data;
      }
      delete float_data;
      dl::leave_critical_section();
    }
  } else if (node.IsSequence()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      mixed data_piece;
      yaml_node_to_mixed(*it, data_piece, source);
      data.push_back(data_piece);
    }
  } else if (node.IsMap()) {
    for (const auto &it : node) {
      mixed data_piece;
      yaml_node_to_mixed(it.second, data_piece, source);
      data[string(it.first.as<std::string>().c_str())] = data_piece;
    }
  }
}

string print_tabs(uint8_t nesting_level) {
  string tabs;
  for (uint8_t i = 0; i < 2 * nesting_level; i++) {
    tabs.push_back(' ');
  }
  return tabs;
}

string print_key(const mixed& data_key) {
  if (data_key.is_string()) {
    return data_key.as_string();
  }
  return string(data_key.as_int()); // array can not be a key; bool and float keys are cast to int
}

void mixed_to_string(const mixed& data, string& string_data, uint8_t nesting_level = 0) {
  if (!data.is_array()) {
    if (data.is_null()) {
      string_data.push_back('~');
    } else if (data.is_string()) {
      const string& string_data_piece = data.as_string();
      if (string_data_piece.size() < 2
          || (string_data_piece[0] != '"' && string_data_piece[string_data_piece.size() - 1] != '"')) {
        string_data.push_back('"');
        string_data.append(string_data_piece);
        string_data.push_back('"');
      } else {
        string_data.append(string_data_piece);
      }
    } else if (data.is_int()) {
      string_data.append(data.as_int());
    } else if (data.is_float()) {
      string_data.append(data.as_double());
    } else if (data.is_bool()) {
      string_data.append((data.as_bool()) ? "true" : "false");
    }
    string_data.push_back('\n');
    return;
  }
  const array<mixed> &data_array = data.as_array();
  if (data_array.is_pseudo_vector()) {
    for (const auto &it : data_array) {
      const mixed &data_piece = it.get_value();
      string_data.append(print_tabs(nesting_level));
      string_data.append("- ");
      if (data_piece.is_array()) {
        string_data.push_back('\n');
      }
      mixed_to_string(data_piece, string_data, nesting_level + 1);
    }
  } else {
    for (const auto &it : data_array) {
      const mixed &data_piece = it.get_value();
      string_data.append(print_tabs(nesting_level));
      string_data.append(print_key(it.get_key()));
      string_data.append(": ");
      if (data_piece.is_array()) {
        string_data.push_back('\n');
      }
      mixed_to_string(data_piece, string_data, nesting_level + 1);
    }
  }
}

bool f$yaml_emit_file(const string &filename, const mixed &data) {
  if (filename.empty()) {
    php_warning("Filename cannot be empty");
    return false;
  }
  string emitted_data = f$yaml_emit(data);
  Optional<int64_t> size = f$file_put_contents(filename, emitted_data);
  if (size.is_false()) {
    php_warning("Error while writing to file \"%s\"", filename.c_str());
    return false;
  }
  return true;
}

string f$yaml_emit(const mixed &data) {
  string string_data;
  mixed_to_string(data, string_data);
  return string_data;
}

mixed f$yaml_parse_file(const string &filename, int pos) {
  if (filename.empty()) {
    php_warning("Filename cannot be empty");
    return {};
  }
  Optional<string> data = f$file_get_contents(filename);
  if (data.is_false()) {
    php_warning("Error while reading file \"%s\"", filename.c_str());
    return {};
  }
  return f$yaml_parse(data.ref(), pos);
}

mixed f$yaml_parse(const string &data, int pos) {
  if (pos != 0) {
    php_warning("Argument \"pos\" = %d. Values other than 0 are not supported yet. Setting to default (pos = 0)", pos);
  }
  YAML::Node node = YAML::Load(data.c_str());
  mixed parsed_data;
  yaml_node_to_mixed(node, parsed_data, data);
  return parsed_data;
}
