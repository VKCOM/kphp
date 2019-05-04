#include "compiler/inferring/key.h"

#include <string>

#include "common/algorithms/hashes.h"

#include "compiler/stage.h"
#include "compiler/threading/format.h"
#include "compiler/threading/hash-table.h"

static TSHashTable<Key *> int_keys_ht;
static TSHashTable<Key *> string_keys_ht;
static TSHashTable<std::string *> string_key_names_ht;
static int n_string_keys_ht = 0;

Key::Key() :
  id(-1) {
}

Key::Key(int id) :
  id(id) {
}

Key Key::any_key() {
  return Key(0);
}

Key Key::string_key(const std::string &key) {
  auto node = string_keys_ht.at(vk::std_hash(key));
  if (node->data != nullptr) {
    return *node->data;
  }

  AutoLocker<Lockable *> locker(node);
  int old_n = __sync_fetch_and_add(&n_string_keys_ht, 1);
  node->data = new Key(old_n * 2 + 2);

  auto name_node = string_key_names_ht.at(node->data->id);
  kphp_assert(name_node->data == nullptr);
  name_node->data = new std::string(key);

  return *node->data;
}

Key Key::int_key(int key) {
  TSHashTable<Key *>::HTNode *node = int_keys_ht.at((unsigned int)key);
  if (node->data != nullptr) {
    return *node->data;
  }

  AutoLocker<Lockable *> locker(node);
  node->data = new Key((unsigned int)key * 2 + 1);
  return *node->data;
}

std::string Key::to_string() const {
  if (is_int_key()) {
    return format("%d", (id - 1) / 2);
  }
  if (is_string_key()) {
    return *string_key_names_ht.at(id)->data;
  }
  if (is_any_key()) {
    return "Any";
  }
  kphp_assert(0);
  return "fail";
}
