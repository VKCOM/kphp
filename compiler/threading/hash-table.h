#pragma once

#include <cassert>
#include <vector>

#include "compiler/threading/locks.h"

template<class T>
class TSHashTable {
  static const int N = 1000000;
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
  int nodes_size;
public:
  TSHashTable() :
    nodes(new HTNode[N]),
    used_size(0),
    nodes_size(N) {
  }

  HTNode *at(unsigned long long hash) {
    int i = (unsigned)hash % (unsigned)nodes_size;
    while (true) {
      while (nodes[i].hash != 0 && nodes[i].hash != hash) {
        i++;
        if (i == nodes_size) {
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

  std::vector<T> get_all() {
    std::vector<T> res;
    for (int i = 0; i < N; i++) {
      if (nodes[i].hash != 0) {
        res.push_back(nodes[i].data);
      }
    }
    return res;
  }
};
