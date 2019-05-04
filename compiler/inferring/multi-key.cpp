#include "compiler/inferring/multi-key.h"

#include "compiler/stage.h"

MultiKey::MultiKey(const std::vector<Key> &keys) :
  keys_(keys) {
}

MultiKey::MultiKey(const MultiKey &multi_key) :
  keys_(multi_key.keys_) {
}

MultiKey &MultiKey::operator=(const MultiKey &multi_key) {
  if (this == &multi_key) {
    return *this;
  }
  keys_ = multi_key.keys_;
  return *this;
}

void MultiKey::push_back(const Key &key) {
  keys_.push_back(key);
}

void MultiKey::push_front(const Key &key) {
  keys_.insert(keys_.begin(), key);
}

std::string MultiKey::to_string() const {
  std::string res;
  for (Key key : *this) {
    res += "[";
    res += key.to_string();
    res += "]";
  }
  return res;
}

MultiKey::iterator MultiKey::begin() const {
  return keys_.begin();
}

MultiKey::iterator MultiKey::end() const {
  return keys_.end();
}

MultiKey::reverse_iterator MultiKey::rbegin() const {
  return keys_.rbegin();
}

MultiKey::reverse_iterator MultiKey::rend() const {
  return keys_.rend();
}

std::vector<MultiKey> MultiKey::any_key_vec;

void MultiKey::init_static() {
  std::vector<Key> keys;
  for (int i = 0; i < 10; i++) {
    any_key_vec.push_back(MultiKey(keys));
    keys.push_back(Key::any_key());
  }
}

const MultiKey &MultiKey::any_key(int depth) {
  if (depth >= 0 && depth < (int)any_key_vec.size()) {
    return any_key_vec[depth];
  }
  kphp_fail();
  return any_key_vec[0];
}
