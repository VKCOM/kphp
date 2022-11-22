#include "parsing_functions.h"

string clearSpecSymbols(const string &str)
{
  return string(std::regex_replace(str.c_str(),std::regex(R"([\t\r\b\v])"), "").c_str());
}

string clearSpaces(const string &str)
{
  return string(std::regex_replace(str.c_str(), std::regex(" += +"), "=").c_str());
}

string clearEOL(const string &str)
{
  return string(std::regex_replace(str.c_str(), std::regex("\\n"), " ").c_str());
}

string clearQuotes(const string &str)
{
  return string(std::regex_replace(str.c_str(), std::regex("[\"\']"), "").c_str());
}

string clearString(const string &str)
{
  string clear_string = clearSpecSymbols(str);
  clear_string = clearSpaces(clear_string);
  clear_string = clearQuotes(clear_string);
  clear_string = trim(clear_string);

  return clear_string;
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

bool isEnvComment(const string &env_comment)
{
  return std::regex_match(env_comment.c_str(), std::regex("^#.*", std::regex::ECMAScript));
}

bool isEnvVar(const string &env_var)
{
  return std::regex_match(env_var.c_str(), std::regex("^[A-Z]+[A-Z\\W\\d_]*$", std::regex::ECMAScript));
}

bool isEnvVal(const string &env_val)
{
  return std::regex_match(env_val.c_str(), std::regex("(.*\n(?=[A-Z])|.*$)", std::regex::ECMAScript));
}

string get_env_var(const string &env_entry)
{
  for (string::size_type i = 0; i < env_entry.size(); i++) {
    if (env_entry[i] == '=') {
      return env_entry.substr(0, i);
    }
  }

  return {};
}

string get_env_val(const string &env_entry)
{
  for (string::size_type i = 0; i < env_entry.size(); i++) {
    if (env_entry[i] == '=') {
      return env_entry.substr(i+1, env_entry.size() - (i+1));
    }
  }

  return {};
}

array<string> f$parse_env_file(const string &filename)
{
  if (filename.empty()) {
    return {};
  }

  std::ifstream ifs(filename.c_str());

  if (!ifs.is_open()) {
    return {};
  }

  array<string> res(array_size(1, 0, true));

  std::string env_entry;
  while (getline(ifs, env_entry)) {
    string env_entry_copy = clearString(string(env_entry.c_str()));

    if (!env_entry_copy.empty() && !isEnvComment(env_entry_copy)) {
      string env_var = get_env_var(env_entry_copy);

      if (env_var.empty()) {
        php_warning("Invalid env string format %s", env_entry_copy.c_str());
        return {};
      }

      string env_val = get_env_val(env_entry_copy);

      if (isEnvVar(env_var) && isEnvVal(env_val)) {
        res.set_value(env_var, env_val);
      } else {
        php_warning("Invalid env string format %s", env_entry_copy.c_str());
        return {};
      }
    }
  }

  ifs.close();

  return res;
}

array<string> f$parse_env_string(const string &env_string)
{
  array<string> res(array_size(1, 0, true));

  string env_string_copy = clearString(env_string);
  env_string_copy = clearEOL(env_string_copy);

  string::size_type len = env_string_copy.size();
  string::size_type s = 0;

  string env_entry;

  for (string::size_type i = 0; i <= len; i++) {
    if (env_string_copy[i] == ' ' || i == len) {
      env_entry = env_string_copy.substr(s, i-s);
      s = i+1;

      if (!isEnvComment(env_entry)) {
        string env_var = get_env_var(env_entry);

        if (env_var.empty()) {
          php_warning("Invalid env string format %s", env_entry.c_str());
          return {};
        }

        string env_val = get_env_val(env_entry);

        if (isEnvVar(env_var) && isEnvVal(env_val)) {
          res.set_value(env_var, env_val);
        } else {
          php_warning("Invalid env string format %s", env_entry.c_str());
          return {};
        }
      }
    }
  }

  return res;
}
