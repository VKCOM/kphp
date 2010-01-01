#include "name-gen.h"
#include "stage.h"
#include "io.h"
#include "data.h"

string register_unique_name (const string &prefix) {
  //static set <string> v;
  //static volatile int x = 0;
  //AutoLocker <volatile int *> locker (&x);
  //if (v.count (prefix)) {
    //fprintf (stderr, "%s\n", prefix.c_str());
    //assert (0);
  //}
  //v.insert (prefix);
  return prefix;
}

string gen_const_string_name (const string &str) {
  AUTO_PROF (next_const_string_name);
  unsigned long long h = hash_ll (str);
  char tmp[50];
  sprintf (tmp, "const_string$us%llx", h);
  return tmp;
}

string gen_const_regexp_name (const string &str) {
  AUTO_PROF (next_const_string_name);
  unsigned long long h = hash_ll (str);
  char tmp[50];
  sprintf (tmp, "const_regexp$us%llx", h);
  return tmp;
}

string gen_unique_name (string prefix, bool flag) {
  for (int i = 0; i < (int)prefix.size(); i++) {
    int c = prefix[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
    } else {
      prefix[i] = '_';
    }
  }
  prefix += "$u";
  AUTO_PROF (next_name);
  FunctionPtr function = stage::get_function();
  if (function.is_null() || flag) {
    return register_unique_name (prefix);
  }
  map <long long, int> *name_gen_map = &function->name_gen_map;
  int h = hash (function->name);
  long long ph = hash_ll (prefix);
  int *i = &(*name_gen_map)[ph];
  int cur_i = (*i)++;
  char tmp[50];
  sprintf (tmp, "%x_%d", h, cur_i);
  return register_unique_name (prefix + tmp);
}

