#pragma once

#include <vector>

#include "compiler/inferring/key.h"

class MultiKey {
private:
  std::vector<Key> keys_;
public:
  using iterator = std::vector<Key>::const_iterator;
  using reverse_iterator = std::vector<Key>::const_reverse_iterator;
  MultiKey() = default;
  MultiKey(const MultiKey &multi_key);
  MultiKey &operator=(const MultiKey &multi_key);
  explicit MultiKey(const std::vector<Key> &keys);
  void push_back(const Key &key);
  void push_front(const Key &key);
  std::string to_string() const;

  inline unsigned int depth() const { return (unsigned int)keys_.size(); }

  iterator begin() const;
  iterator end() const;
  reverse_iterator rbegin() const;
  reverse_iterator rend() const;

  static std::vector<MultiKey> any_key_vec;
  static void init_static();
  static const MultiKey &any_key(int depth);
};
