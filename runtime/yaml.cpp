#include <yaml-cpp/yaml.h>

#include "runtime/optional.h"
#include "runtime/streams.h"
#include "runtime/yaml.h"

/*
 * convert YAML::Node to mixed after parsing a YAML document into YAML::Node
 */
static void yaml_node_to_mixed(const YAML::Node &node, mixed &data, const string &source) noexcept {
  data.clear(); // sets data to NULL
  if (node.IsScalar()) {
    const string string_data(node.as<std::string>().c_str());
    // check whether the primitive is put in quotes in the source YAML
    const bool string_data_has_quotes =
      (source[node.Mark().pos] == '"' && source[node.Mark().pos + string_data.size() + 1] == '"') ||
      (source[node.Mark().pos] == '\'' && source[node.Mark().pos + string_data.size() + 1] == '\'');
    // if so, it is a string
    if (string_data_has_quotes) {
      data = string_data;
    } else if (string_data == string("true")) {
      data = true; // "true" without quotes is boolean(1)
    } else if (string_data == string("false")) {
      data = false; // "false" without quotes is boolean(0)
    } else if (string_data.is_int()) {
      data = string_data.to_int();
    } else {
      double float_data = 0.0;
      if (string_data.try_to_float(&float_data)) {
        data = float_data;
      } else {
        data = string_data;
      }
    }
  } else if (node.size() == 0 && node.IsDefined() && !node.IsNull()) {
    // if node is defined, is not null or scalar and has size 0, then it is an empty array
    array<mixed> empty_array;
    data = empty_array;
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
      data.set_value(string(it.first.as<std::string>().c_str()), data_piece);
    }
  }
  // else node is Null or Undefined, so data is Null
}

/*
 * print tabs in quantity of nesting_level (used to print nested YAML entries)
 */
static string yaml_print_tabs(const uint8_t nesting_level) noexcept {
  return string(2 * nesting_level, ' ');
}

/*
 * print the key of a YAML map entry
 */
static string yaml_print_key(const mixed &data_key) noexcept {
  if (data_key.is_string()) {
    return data_key.as_string();
  }
  // key can not be an array; bool and float keys are cast to int
  return string(data_key.as_int());
}

/*
 * escape special characters in a string entry
 */
static string yaml_escape(const string &data) noexcept {
  string escaped_data;
  for (size_t i = 0; i < data.size(); ++i) {
    const char& current_char = data[i];
    if (current_char == '\n') { // line feed
      escaped_data.push_back('\\');
      escaped_data.push_back('n');
    } else if (current_char == '\b') { // backspace
      escaped_data.push_back('\\');
      escaped_data.push_back('b');
    } else if (current_char == '\t') { // horizontal tab
      escaped_data.push_back('\\');
      escaped_data.push_back('t');
    } else if (current_char == '\v') { // vertical tab
      escaped_data.push_back('\\');
      escaped_data.push_back('v');
    } else if (current_char == '\"') {
      escaped_data.push_back('\\');
      escaped_data.push_back('"');
    } else if (current_char == '\\') {
      escaped_data.push_back('\\');
      escaped_data.push_back('\\');
    } else {
      escaped_data.push_back(current_char);
    }
  }
  return escaped_data;
}

/*
 * get a YAML representation of mixed in a string variable
 */
static void mixed_to_string(const mixed &data, string &string_data, const uint8_t nesting_level = 0) noexcept {
  if (!data.is_array()) {
    if (data.is_null()) {
      string_data.push_back('~'); // tilda is a YAML representation of NULL
    } else if (data.is_string()) {
      string_data.push_back('"'); // cover string entry in double quotes
      string_data.append(yaml_escape(data.as_string())); // escape special characters
      string_data.push_back('"');
    } else if (data.is_int()) {
      string_data.append(data.as_int());
    } else if (data.is_float()) {
      string_data.append(data.as_double());
    } else if (data.is_bool()) {
      const string bool_repr = (data.as_bool()) ? string("true") : string("false");
      string_data.append(bool_repr);
    }
    string_data.push_back('\n');
    return;
  }
  const array<mixed> &data_array = data.as_array();
  if (data_array.empty()) {
    string_data.append("[]\n"); // an empty array is represented as [] in YAML
    return;
  }
  // check if an array has keys increasing by 1 starting from 0
  const bool data_array_is_vector = data_array.is_pseudo_vector();
  for (const auto &it : data_array) {
    const mixed &data_piece = it.get_value();
    string_data.append(yaml_print_tabs(nesting_level));
    if (data_array_is_vector) {
      string_data.push_back('-');
    } else {
      string_data.append(yaml_print_key(it.get_key()));
      string_data.push_back(':');
    }
    if (data_piece.is_array() && !data_piece.as_array().empty()) {
      // if an element of an array is also a non-empty array, print it on the next line
      string_data.push_back('\n');
    } else {
      // if an element of an array is a primitive or an empty array, print it after a space
      string_data.push_back(' ');
    }
    // for entries of an array, increase nesting level
    mixed_to_string(data_piece, string_data, nesting_level + 1);
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
  string string_data("---\n"); // beginning of a YAML document
  mixed_to_string(data, string_data);
  string_data.append("...\n"); // ending of a YAML document
  return string_data;
}

mixed f$yaml_parse_file(const string &filename, int pos) {
  if (filename.empty()) {
    php_warning("Filename cannot be empty");
    return false;
  }
  Optional<string> data = f$file_get_contents(filename);
  if (data.is_false()) {
    php_warning("Error while reading file \"%s\"", filename.c_str());
    return false;
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
