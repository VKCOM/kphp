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
    std::atomic<unsigned long long> hash;
    T data;

    HTNode() :
      hash(0),
      data() {
    }
  };

private:
  HTNode *nodes;
  std::atomic<int> used_size;
public:
  TSHashTable() :
    nodes(new HTNode[N]),
    used_size(0) {
  }

  HTNode *at(unsigned long long hash) {
    int i = (unsigned)hash % (unsigned)N;
    while (true) {
      while (nodes[i].hash.load(std::memory_order_acquire) != 0 
        && nodes[i].hash.load(std::memory_order_relaxed) != hash) {
        i++;
        if (i == N) {
          i = 0;
        }
      }
      unsigned long long expected = 0;
      if (nodes[i].hash.load(std::memory_order_acquire) == 0 
        && nodes[i].hash.compare_exchange_weak(expected, hash, std::memory_order_acq_rel)) {
        int id = used_size.fetch_add(1, std::memory_order_release);
        assert(id * 2 < N);
        continue;
      }
      break;
    }
    return &nodes[i];
  }

  const T *find(unsigned long long hash) {
    int i = (unsigned)hash % (unsigned)N;
    while (nodes[i].hash.load(std::memory_order_acquire) != 0 
      && nodes[i].hash.load(std::memory_order_relaxed) != hash) {
      i++;
      if (i == N) {
        i = 0;
      }
    }

    return nodes[i].hash.load(std::memory_order_acquire) == hash ? &nodes[i].data : nullptr;
  }

  std::vector<T> get_all() {
    std::vector<T> res;
    for (int i = 0; i < N; i++) {
      if (nodes[i].hash.load(std::memory_order_acquire) != 0) {
        res.push_back(nodes[i].data);
      }
    }
    return res;
  }

  template<class CondF>
  std::vector<T> get_all_if(const CondF &callbackF) {
    std::vector<T> res;
    for (int i = 0; i < N; i++) {
      if (nodes[i].hash.load(std::memory_order_acquire) != 0 && callbackF(nodes[i].data)) {
        res.push_back(nodes[i].data);
      }
    }
    return res;
  }
};
