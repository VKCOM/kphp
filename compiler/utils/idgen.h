// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/utils/idmap.h"

template<class IdType>
struct IdGen {
  using iterator = typename IdMap<IdType>::iterator;

  vector<IdMapBase *> id_maps;
  IdMap<IdType> ids;
  int n{0};

  void add_id_map(IdMapBase *to_add);

  int init_id(IdType *to_add);
  int size();
  iterator begin();
  iterator end();
  void clear();
};

template<class IdType>
void IdGen<IdType>::add_id_map(IdMapBase *to_add) {
  to_add->update_size((n | 0xff) + 1);
  id_maps.push_back(to_add);
}

template<class IdType>
int IdGen<IdType>::init_id(IdType *to_add) {
  set_index(*to_add, n);

  if ((n & 0xff) == 0) {
    int real_n = (n | 0xff) + 1;
    ids.update_size(real_n);

    for (int i = 0; i < (int)id_maps.size(); i++) {
      id_maps[i]->update_size(real_n);
    }
  }

  ids[*to_add] = *to_add;
  n++;
  return n - 1;
}

template<class IdType>
int IdGen<IdType>::size() {
  return n;
}

template<class IdType>
typename IdGen<IdType>::iterator IdGen<IdType>::begin() {
  return ids.begin();
}

template<class IdType>
typename IdGen<IdType>::iterator IdGen<IdType>::end() {
  return ids.begin() + size();
}

template<class IdType>
void IdGen<IdType>::clear() {
  n = 0;
  for (auto &x: ids) {
    x.clear();
  }
  ids.clear();
  for (int i = 0; i < (int)id_maps.size(); i++) {
    id_maps[i]->clear();
  }
  id_maps.clear();
}
