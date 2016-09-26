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
#include "common-php-functions.h"

size_t total_mem_used __attribute__ ((weak));

template <class T, class TrackType, class BaseAllocator = std::allocator <T> >
class TrackerAllocator : public BaseAllocator {
public:
  typedef typename BaseAllocator::pointer pointer;
  typedef typename BaseAllocator::size_type size_type;

  TrackerAllocator() throw()
    : BaseAllocator() {
  }

  TrackerAllocator (const TrackerAllocator& b) throw()
    : BaseAllocator (b) {
  }

  template <class U>
  TrackerAllocator (const typename TrackerAllocator::template rebind <U>::other& b) throw()
    : BaseAllocator (b) {
  }

  ~TrackerAllocator() {
  }

  template <class U> struct rebind {
    typedef TrackerAllocator <U, TrackType, typename BaseAllocator::template rebind <U>::other> other;
  };

  pointer allocate (size_type n) {
    pointer r = BaseAllocator::allocate (n);
    total_mem_used += n;
    return r;
  }

  pointer allocate (size_type n, pointer h) {
    pointer r = BaseAllocator::allocate (n, h);
    total_mem_used += n;
    return r;
  }

  void deallocate (pointer p, size_type n) throw() {
    BaseAllocator::deallocate (p, n);
    total_mem_used -= n;
  }
};

//typedef std::basic_string <char,
                           //std::char_traits<char>,
                           //TrackerAllocator<char, std::string> > string;
//typedef std::vector<int,
                    //TrackerAllocator<int, std::vector<int> > > trackvector;
using std::vector;
using std::cerr;
using std::endl;
using std::map;
using std::set;
using std::tr1::shared_ptr;
using std::pair;
using std::queue;
using std::pair;
using std::make_pair;
using std::stringstream;
using std::string;
using std::swap;
using std::max;
using std::min;
using std::priority_queue;

#define DL_ADD_SUFF_(A, B) A##_##B
#define DL_ADD_SUFF(A, B) DL_ADD_SUFF_(A, B)

#define DL_CAT_(A, B) A##B
#define DL_CAT(A, B) DL_CAT_(A, B)
#define DL_STR(A) DL_STR_(A)
#define DL_STR_(A) # A
#define DL_EMPTY(A...)

#define DISALLOW_COPY_AND_ASSIGN(type_name) \
  type_name (const type_name&);             \
  void operator = (const type_name&);


bool use_safe_integer_arithmetic __attribute__ ((weak)) = false;

inline int hash (const string &s) {
  int res = 31;
  for (int i = 0; i < (int)s.size(); i++) {
    res = res * 239 + s[i];
  }
  res &= ~((unsigned)1 << 31);
  return res;
}

inline unsigned long long hash_ll (const string &s) {
  unsigned long long res = 31;
  for (int i = 0; i < (int)s.size(); i++) {
    res = res * 239 + s[i];
  }
  return res;
}

