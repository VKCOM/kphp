#include "ini.h"

/*
 * INI parsing functions
 */

const int64_t SINGLE_QOUTES = 0;
const int64_t DOUBLE_QUOTES = 1;
const int64_t ALL_QUOTES = 2;

string clear_quotes(const string &str, int64_t flag) {
  if (str.empty()) {
    return {};
  }

  Optional<string> clear_str;

  switch (flag) {
    case SINGLE_QOUTES:
        clear_str = f$preg_replace(string("/'+/"), string(""), str);
      break;
    case DOUBLE_QUOTES:
        clear_str = f$preg_replace(string("/\"+/"), string(""), str);
      break;
    case ALL_QUOTES:
        clear_str = f$preg_replace(string("/['|\"]+/"), string(""), str);
      break;
  }

  if (clear_str.is_null()) {
    return string("");
  }

  return clear_str.ref();
}

string clear_extra_spaces(const string &str) {
  if (str.empty()) {
    return {};
  }

  Optional<string> clear_str;

  clear_str = f$preg_replace(string("/ +/"), string(" "), str);

  if (clear_str.is_null()) {
    return string("");
  }

  return clear_str.ref();
}

/*
 * INI parsing functions
 */

bool is_ini_section(const string &ini_section) {
  return f$preg_match(string("/^\\[.+\\]$/"), ini_section).val();
}

bool is_ini_var(const string &ini_var) {
  return f$preg_match(string("/^.+/"), ini_var).val();
}

bool is_ini_val(const string &ini_val) {
  return f$preg_match(string("/(.*\n(?=[A-Z])|.*$)/"), ini_val).val();
}

bool is_ini_bool_val(const string &ini_val) {
  return f$preg_match(string("/^(false|true|0|1|off|on)$/i"), ini_val).val();
}

bool is_ini_float_val(const string &ini_val) {
  return f$preg_match(string(R"(/^\d+\.\d*$/)"), ini_val).val();
}

bool is_ini_str_val(const string &ini_val) {
  return f$preg_match(string("/^.*$/i"), ini_val).val();
}

string get_ini_section(const string &ini_entry) {
  if (ini_entry.empty()) {
    return {};
  }

  return ini_entry.substr(1, ini_entry.size()-2);
}

array<string> split_ini_entry(const string &ini_entry) {
  if (ini_entry.empty()) {
    return {};
  }

  array<string> res = f$explode(string("="), ini_entry, 2);

  if (res.size().int_size != 2) {
    php_warning("Error");
    return {};
  }

  return res;
}

bool cast_str_to_bool(const string &ini_var) {
  if (ini_var.empty()) {
    return false;
  }

  string ini_bool_var = f$strtolower(ini_var);

  if (ini_bool_var == string("on") || ini_bool_var == string("1") || ini_bool_var == string("true")) {
    return true;
  }

  if (ini_bool_var == string("off") || ini_bool_var == string("0") || ini_bool_var == string("false")) {
    return false;
  }

  return false;
}

array<mixed> f$parse_ini_string(const string &ini_string, bool process_sections, int scanner_mode) {
  if (ini_string.empty()) {
    return {};
  }

  string ini_string_copy = f$trim(ini_string);
  ini_string_copy = clear_extra_spaces(ini_string_copy);

  array<mixed> res(array_size(0, 0, true));
  array<mixed> section(array_size(0, 0, true));
  string ini_entry;
  string ini_section;

  for (string::size_type i = 0; i <= ini_string_copy.size(); ++i) {
    if (ini_string_copy[i] == '[') {
      while (ini_string_copy[i] != ']') {
        ini_entry.push_back(ini_string_copy[i]);
        ++i;
      }
    }

    if (ini_string_copy[i] == '\"' || ini_string_copy[i] == '\'') {
      ini_entry.push_back(ini_string_copy[i]);

      ++i;
      while (ini_string_copy[i] != '\"' && ini_string_copy[i] != '\'') {
        ini_entry.push_back(ini_string_copy[i]);
        ++i;
      }
    }

    if (ini_string_copy[i] == ' ' || ini_string_copy[i] == '\n' || ini_string_copy[i] == '\0') {
      if (is_ini_section(ini_entry)) {
        if (process_sections && !ini_section.empty()) {
          res.set_value(ini_section, section);
          section.clear();
        }

        ini_section = get_ini_section(ini_entry);
      } else if (!ini_entry.empty()){
        array<string> ini = split_ini_entry(ini_entry);

        if (ini.size().int_size != 2) {
          php_warning("Invalid ini string format %s", ini_entry.c_str());
          return {};
        }

        string ini_var = ini[0];
        string ini_val = ini[1];

        if (!is_ini_var(ini_var) && !is_ini_val(ini_val)) {
          php_warning("Invalid ini string format %s", ini_entry.c_str());
          return {};
        }

        if (!ini_var.empty()) {
          switch (scanner_mode) {
            case INI_SCANNER_NORMAL:
              if (is_ini_bool_val(ini_val)) {
                section.set_value(ini_var, cast_str_to_bool(ini_val) ? string("1") : string(""));
              } else if (is_ini_str_val(ini_val)) {
                section.set_value(ini_var, clear_quotes(ini_val, ALL_QUOTES));
              }
              break;
            case INI_SCANNER_RAW:
              if (is_ini_str_val(ini_val)) {
                section.set_value(ini_var, clear_quotes(ini_val, DOUBLE_QUOTES));
              } else {
                section.set_value(ini_var, ini_val);
              }
              break;
            case INI_SCANNER_TYPED:
              if (ini_val.is_int()) {
                section.set_value(ini_var, ini_val.to_int());
              } else if (is_ini_float_val(ini_val)) {
                section.set_value(ini_var, ini_val.to_float());
              } else if (is_ini_bool_val(ini_val)) {
                section.set_value(ini_var, cast_str_to_bool(ini_val));
              } else if (is_ini_str_val(ini_val)) {
                section.set_value(ini_var, clear_quotes(ini_val, ALL_QUOTES));
              }
              break;
          }
        }
      }

      ini_entry = {};
    } else {
      ini_entry.push_back(ini_string_copy[i]);
    }
  }

  if (process_sections) {
    if (!ini_section.empty()) {
      res.set_value(ini_section, section);
    }
  } else {
    return section;
  }

  return res;
}



array<mixed> f$parse_ini_file(const string &filename, bool process_sections, int scanner_mode) {
  if (filename.empty()) {
    php_warning("Filename cannot be empty");
    return {};
  }

  Optional<string> ini_string = f$file_get_contents(filename);

  if (ini_string.is_null()) {
    return {};
  }

  return f$parse_ini_string(ini_string.ref(), process_sections, scanner_mode);
}
