#pragma once

#include "runtime/kphp_core.h"

typedef int curl;

curl f$curl_init (const string &url = string());

bool f$curl_set_header_function (curl id, var (*header_function) (var curl_id, var string));

bool f$curl_setopt (curl id, int option, const var &value);

bool f$curl_setopt_array (curl id, const array <var> &options);

template <class T>
bool f$curl_setopt_array (curl id, const array <T> &options);

var f$curl_exec (curl id);

var f$curl_getinfo (curl id, int option = 0);

string f$curl_error (curl id);

int f$curl_errno (curl id);

void f$curl_close (curl id);


void curl_free_static (void);


/*
 *
 *     IMPLEMENTATION
 *
 */


template <class T>
bool f$curl_setopt_array (curl id, const array <T> &options) {
  return f$curl_setopt_array (id, array <var> (options));
}
