#include "parsing_functions.h"

/*
 * Helper functions
 */

string clearQuotes(const string &str)
{
  if (str.starts_with(string("\'"))) {
    return clearSingleQuotes(str);
  }

  if (str.starts_with(string("\""))) {
    return clearDoubleQuotes(str);
  }

  return str;
}

string clearSingleQuotes(const string &str)
{
  if (str.empty()) {
    return {};
  }

  if (str.starts_with(string("\'")) && str.ends_with(string("\'"))) {
    return str.substr(1, str.size()-2);
  }

  return str;
}

string clearDoubleQuotes(const string &str)
{
  if (str.empty()) {
    return {};
  }

  if (str.starts_with(string("\"")) && str.ends_with(string("\""))) {
    return str.substr(1, str.size()-2);
  }

  return str;
}

string clearExtraSpaces(const string &str)
{
  if (str.empty()) {
    return {};
  }

  string clear_str;

  for (string::size_type i = 0; i < str.size(); i++) {
    if (str[i] != ' ') {
      clear_str.push_back(str[i]);
    } else {
      clear_str.push_back(' ');

      while (str[i+1] == ' ') {
        i++;
      }
    }
  }

  return clear_str;
}

string tolower(const string &str)
{
  if (str.empty()) {
    return {};
  }

  string lower_str = {};

  for (string::size_type i = 0; i < str.size(); i++) {
    lower_str.push_back(tolower(str[i]));
  }

  return lower_str;
}

string trim(const string &str)
{
  if (str.empty()) {
    return {};
  }

  size_t s = 0;
  size_t e = str.size()-1;

  while (s != e && std::isspace(str[s])) {
    s++;
  }

  while (e != s && std::isspace(str[e])) {
    e--;
  }

  return str.substr(s, (e-s)+1);
}

/*
 * INI parsing functions
 */

bool isINISection(const string &ini_section)
{
  return std::regex_match(ini_section.c_str(), std::regex("^\\[.+\\]$", std::regex::ECMAScript));
}

bool isINIVar(const string &ini_var)
{
  return std::regex_match(ini_var.c_str(), std::regex("^.+", std::regex::ECMAScript));
}

bool isINIVal(const string &ini_val)
{
  return std::regex_match(ini_val.c_str(), std::regex("(.*\n(?=[A-Z])|.*$)", std::regex::ECMAScript));
}

bool isINIBoolVal(const string &ini_val)
{
  return std::regex_match(ini_val.c_str(), std::regex("^(false|true|0|1|off|on)$", std::regex::ECMAScript|std::regex::icase));
}

bool isINIFloatVal(const string &ini_val)
{
  return std::regex_match(ini_val.c_str(), std::regex(R"(^\d+\.\d*$)", std::regex::ECMAScript|std::regex::icase));
}

bool isINIStrVal(const string &ini_val)
{
  return std::regex_match(ini_val.c_str(), std::regex("^.*$", std::regex::ECMAScript|std::regex::icase));
}

string get_ini_section(const string &ini_entry)
{
  if (ini_entry.empty()) {
    return {};
  }

  return ini_entry.substr(1, ini_entry.size()-2);
}

string get_ini_var(const string &ini_entry)
{
  string::size_type pos = ini_entry.find_first_of(string("="), 0);

  if (pos == string::npos) {
    return {};
  }

  return ini_entry.substr(0, pos);

}

string get_ini_val(const string &ini_entry)
{
  string::size_type pos = ini_entry.find_first_of(string("="), 0);

  if (pos == string::npos) {
    return {};
  }

  pos++;

  return ini_entry.substr(pos, ini_entry.size() - pos);
}


bool cast_str_to_bool(const string &ini_var)
{
  if (ini_var.empty()) {
    return false;
  }

  string ini_bool_var = tolower(ini_var);

  if (ini_bool_var == string("on") || ini_bool_var == string("1") || ini_bool_var == string("true")) {
    return true;
  }

  if (ini_bool_var == string("off") || ini_bool_var == string("0") || ini_bool_var == string("false")) {
    return false;
  }

  return false;
}

array<mixed> f$parse_ini_string(const string &ini_string, bool process_sections, int scanner_mode)
{
  if (ini_string.empty()) {
    return {};
  }

  string ini_string_copy = trim(ini_string);
  ini_string_copy = clearExtraSpaces(ini_string_copy);

  array<mixed> res(array_size(0, 0, true));
  array<mixed> section(array_size(0, 0, true));
  string ini_entry;
  string ini_section;

  for (string::size_type i = 0; i <= ini_string_copy.size(); i++) {
    if (ini_string_copy[i] == '[') {
      while (ini_string_copy[i] != ']') {
        ini_entry.push_back(ini_string_copy[i]);
        i++;
      }
    }

    if (ini_string_copy[i] == '\"' || ini_string_copy[i] == '\'') {
      ini_entry.push_back(ini_string_copy[i]);

      i++;
      while (ini_string_copy[i] != '\"' && ini_string_copy[i] != '\'') {
        ini_entry.push_back(ini_string_copy[i]);
        i++;
      }
    }

    if (ini_string_copy[i] == ' ' || ini_string_copy[i] == '\n' || ini_string_copy[i] == '\0') {
      if (isINISection(ini_entry)) {
        if (process_sections && !ini_section.empty()) {
          res.set_value(ini_section, section);
          section.clear();
        }

        ini_section = get_ini_section(ini_entry);
      } else {
        string ini_var = get_ini_var(ini_entry);
        string ini_val = get_ini_val(ini_entry);

        if (!isINIVar(ini_var) && !isINIVal(ini_val)) {
          php_warning("Invalid ini string format %s", ini_entry.c_str());
          return {};
        }

        if (!ini_var.empty() && !ini_val.empty()) {
          switch (scanner_mode) {
            case INI_SCANNER_NORMAL:
              if (isINIBoolVal(ini_val)) {
                section.set_value(ini_var, cast_str_to_bool(ini_val) ? string("1") : string(""));
              } else if (isINIStrVal(ini_val)) {
                section.set_value(ini_var, clearQuotes(ini_val));
              }

              break;
            case INI_SCANNER_RAW:
              if (isINIStrVal(ini_val)) {
                section.set_value(ini_var, clearDoubleQuotes(ini_val));
              } else {
                section.set_value(ini_var, ini_val);
              }

              break;
            case INI_SCANNER_TYPED:
              if (ini_val.is_int()) {
                section.set_value(ini_var, ini_val.to_int());
              } else if (isINIFloatVal(ini_val)) {
                section.set_value(ini_var, ini_val.to_float());
              } else if (isINIBoolVal(ini_val)) {
                section.set_value(ini_var, cast_str_to_bool(ini_val));
              } else if (isINIStrVal(ini_val)) {
                section.set_value(ini_var, clearQuotes(ini_val));
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
