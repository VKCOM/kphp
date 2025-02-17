// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <vector>

#include "compiler/threading/locks.h"

template<class T, int N = 1000000>
class TSHashTable {
public:
  struct HTNode : Lockable {
    unsigned long long hash;
    T data;

    HTNode() :
      hash(0),
      data() {
    }
  };

private:
  HTNode *nodes;
  int used_size;
public:
  TSHashTable() :
    nodes(new HTNode[N]),
    used_size(0) {
  }

  HTNode *at(unsigned long long hash) {
    int i = (unsigned)hash % (unsigned)N;
    while (true) {
      while (nodes[i].hash != 0 && nodes[i].hash != hash) {
        i++;
        if (i == N) {
          i = 0;
        }
      }
      if (nodes[i].hash == 0 && !__sync_bool_compare_and_swap(&nodes[i].hash, 0, hash)) {
        int id = __sync_fetch_and_add(&used_size, 1);
        assert(id * 2 < N);
        continue;
      }
      break;
    }
    return &nodes[i];
  }

  const T *find(unsigned long long hash) {
    int i = (unsigned)hash % (unsigned)N;
    while (nodes[i].hash != 0 && nodes[i].hash != hash) {
      i++;
      if (i == N) {
        i = 0;
      }
    }

    return nodes[i].hash == hash ? &nodes[i].data : nullptr;
  }

  std::vector<T> get_all() {
    std::vector<T> res;
    res.reserve(used_size);
    for (int i = 0; i < N; i++) {
      if (nodes[i].hash != 0) {
        res.push_back(nodes[i].data);
      }
    }
    return res;
  }

  template<class CondF>
  std::vector<T> get_all_if(const CondF &callbackF) {
    std::vector<T> res;
    for (int i = 0; i < N; i++) {
      if (nodes[i].hash != 0 && callbackF(nodes[i].data)) {
        res.push_back(nodes[i].data);
      }
    }
    return res;
  }

  constexpr int max_size() const {
    return N;
  }

  int get_used_size() const {
    return used_size;
  }
};
