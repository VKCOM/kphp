#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <map>
#include <mcheck.h>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <tr1/memory>
#include <typeinfo>
#include <unistd.h>
#include <vector>

#include "drinkless/dl-utils-lite.h"

#define dl_pstr don_t_use_dl_pstr_use_format

#include "common-php-functions.h"

size_t total_mem_used __attribute__ ((weak));

using std::vector;
using std::pair;
using std::map;
using std::set;
using std::queue;
using std::stringstream;
using std::string;

bool use_safe_integer_arithmetic __attribute__ ((weak)) = false;

template<class K, class V>
std::vector<V> get_map_values(const std::map<K, V> &m) {
  std::vector<V> res;
  res.reserve(m.size());

  for (const auto &kv: m) {
    res.emplace_back(kv.second);
  }

  return res;
}

