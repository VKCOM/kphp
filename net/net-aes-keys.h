// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_NET_NET_AES_KEYS_H
#define KDB_NET_NET_AES_KEYS_H

#include <stdbool.h>
#include <stdint.h>

#include "common/kprintf.h"

#define	AES_KEY_MIN_LEN 32
#define AES_KEY_MAX_LEN 256

DECLARE_VERBOSITY(net_crypto_aes);

struct aes_key {
  const char *filename;
  int32_t id;
  int32_t len;
  uint8_t *key;
};
typedef struct aes_key aes_key_t;

extern aes_key_t *default_aes_key;

aes_key_t *create_aes_key();
bool aes_key_add(aes_key_t *aes_key);
void free_aes_key(aes_key_t *key);

aes_key_t *aes_key_load_memory(const char* filename, uint8_t *key, int32_t key_len);
bool aes_key_load_path(const char* path);

aes_key_t *find_aes_key_by_id(int32_t id);
aes_key_t *find_aes_key_by_filename(const char *filename);

#endif // KDB_NET_NET_AES_KEYS_H
