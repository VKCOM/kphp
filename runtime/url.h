#pragma once

#include "kphp_core.h"

string f$base64_decode (const string &s);

string f$base64_encode (const string &s);

template <class T>
string f$http_build_query (const array <T> &a);

void parse_str_set_value (var &arr, const string &key, const string &value);

void f$parse_str (const string &str, var &arr);

var f$parse_url (const string &s, int component = -1);

string f$rawurldecode (const string &s);

string f$rawurlencode (const string &s);

string f$urldecode (const string &s);

string f$urlencode (const string &s);

/*
 *
 *     IMPLEMENTATION
 *
 */

template <class T>
string http_build_query_get_param (const string &key, const T &a) {
  if (f$is_array (a)) {
    string result;
    int first = 1;
    for (typename array <T>::const_iterator p = a.begin(); p != a.end(); ++p) {
      if (!first) {
        result.push_back ('&');
      }
      first = 0;
      result.append (http_build_query_get_param ((static_SB.clean() + key + '[' + p.get_key() + ']').str(), p.get_value()));
    }
    return result;
  } else {
    return (static_SB.clean() + f$urlencode (key) + '=' + f$urlencode (f$strval (a))).str();
  }
}


template <class T>
string f$http_build_query (const array <T> &a) {
  string result;
  int first = 1;
  for (typename array <T>::const_iterator p = a.begin(); p != a.end(); ++p) {
    string param = http_build_query_get_param (f$strval (p.get_key()), p.get_value());
    if (param.size()) {
      if (!first) {
        result.push_back ('&');
      }
      first = 0;

      result.append (param);
    }
  }

  return result;
}
