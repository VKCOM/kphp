// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-aes-keys.h"

#include <assert.h>
#include <climits>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common/io.h"
#include "common/kprintf.h"
#include "common/secure-bzero.h"
#include "common/server/signals.h"
#include "common/wrappers/memory-utils.h"

DEFINE_VERBOSITY(net_crypto_aes)

static_assert(AES_KEY_MIN_LEN >= sizeof(((aes_key_t *) NULL)->id), "key_size");

static aes_key_t **aes_loaded_keys;
static size_t aes_loaded_keys_size;
aes_key_t *default_aes_key;

aes_key_t *create_aes_key() {
  aes_key_t *key = static_cast<aes_key_t*>(calloc(1, sizeof(*key)));
  assert(!posix_memalign((void **) &key->key, 4096, AES_KEY_MAX_LEN));

  our_madvise(key->key, AES_KEY_MAX_LEN, MADV_DONTDUMP);
  return key;
}

void free_aes_key(aes_key_t *key) {
  secure_bzero(key->key, AES_KEY_MAX_LEN);
  free(key->key);
  free(key);
}

bool aes_key_add(aes_key_t *aes_key) {
  for (size_t i = 0; i < aes_loaded_keys_size; ++i) {
    aes_key_t *added_key = aes_loaded_keys[i];

    if (aes_key->id == added_key->id || !strcmp(aes_key->filename, added_key->filename)) {
      tvkprintf(net_crypto_aes, 1, "Cannot add AES key %d(\"%s\"): already added %d(\"%s\")\n", aes_key->id, aes_key->filename, added_key->id, added_key->filename);
      return false;
    }
  }

  aes_loaded_keys = static_cast<aes_key_t**>(realloc(aes_loaded_keys, sizeof(aes_key) * (aes_loaded_keys_size + 1)));
  aes_loaded_keys[aes_loaded_keys_size++] = aes_key;

  tvkprintf(net_crypto_aes, 1, "Add AES key %u(\"%s\")\n", aes_key->id, aes_key->filename);

  return true;
}

static bool aes_key_set_default(const char *filename) {
  if (!default_aes_key) {
    for (size_t i = 0; i < aes_loaded_keys_size; ++i) {
      aes_key_t *key = aes_loaded_keys[i];
      if (!strcmp(key->filename, filename)) {
        tvkprintf(net_crypto_aes, 1, "Setting default AES key to: %d(\"%s\")\n", key->id, key->filename);
        default_aes_key = key;
        return true;
      }
    }
  }

  return false;
}

aes_key_t *aes_key_load_memory(const char* filename, uint8_t *key, int32_t key_len) {
  assert(key_len <= AES_KEY_MAX_LEN);

  aes_key_t *aes_key = create_aes_key();

  aes_key->filename = filename;
  aes_key->len = key_len;
  memcpy(aes_key->key, key, aes_key->len);
  aes_key->id = (*(int32_t *) aes_key->key);

  return aes_key;
}

static aes_key_t *aes_key_load_fd(int fd, const char *filename) {
  struct stat st;
  if (fstat(fd, &st) == -1) {
    tvkprintf(net_crypto_aes, 1, "Cannot fstat() AES key fd: %d(\"%s\"): %m\n", fd, filename);
    return NULL;
  }

  if (st.st_size < AES_KEY_MIN_LEN) {
    tvkprintf(net_crypto_aes, 1, "Ignoring too small AES key: %jd(min %d)\n", (intmax_t)(st.st_size), AES_KEY_MIN_LEN);
    return NULL;
  }

  if (st.st_size > AES_KEY_MAX_LEN) {
    tvkprintf(net_crypto_aes, 1, "Ignoring too large AES key: %jd(max %d)\n", (intmax_t)(st.st_size), AES_KEY_MAX_LEN);
    return NULL;
  }

  char buffer[AES_KEY_MAX_LEN];
  if (!read_exact(fd, buffer, st.st_size)) {
    tvkprintf(net_crypto_aes, 1, "Cannot read AES key fd: %d: %m\n", fd);
    return NULL;
  }

  return aes_key_load_memory(strdup(filename), reinterpret_cast<uint8_t*>(buffer), st.st_size);
}

static bool aes_key_load_file(int fd, const char *path) {
  assert(!default_aes_key);

  //  Both dirname() and basename() may modify the contents of path, so it may be desirable to pass a copy when calling one of these functions.
  char *tmp_path = strdup(path);
  const char *filename = basename(tmp_path);

  aes_key_t *key = aes_key_load_fd(fd, filename);
  close(fd);
  if (!key) {
    free(tmp_path);
    tvkprintf(net_crypto_aes, 1, "Cannot load AES key from fd: %d(\"%s\"): %m\n", fd, path);
    return false;
  }

  const bool loaded = aes_key_add(key) && aes_key_set_default(filename);
  free(tmp_path);

  return loaded;
}

static bool aes_key_load_dir(int fd) {
  DIR *dir = fdopendir(fd);

  struct dirent *entry;
  while ((entry = readdir(dir))) {
    const int dir_fd = dirfd(dir);

    const int fd = openat(dir_fd, entry->d_name, O_NOFOLLOW);
    if (fd == -1) {
      if(errno != ELOOP) {
        tvkprintf(net_crypto_aes, 1, "Cannot openat() AES key dir entry: \"%s\": %m\n", entry->d_name);
      }
      continue;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
      tvkprintf(net_crypto_aes, 1, "Cannot fstatat() AES key dir entry: \"%s\": %m\n", entry->d_name);
      continue;
    }

    if (S_ISREG(st.st_mode)) {
      aes_key_t *key = aes_key_load_fd(fd, entry->d_name);
      if (key) {
        aes_key_add(key);
      }
    }
  }

  const int dir_fd = dirfd(dir);
  char buffer[NAME_MAX + 1];
  if (readlinkat(dir_fd, "default", buffer, sizeof(buffer)) == -1) {
    assert(!closedir(dir));
    tvkprintf(net_crypto_aes, 1, "Cannot readlinkat() \"default\" AES key symlink\n");
    return false;
  }
  assert(!closedir(dir));

  return aes_key_set_default(buffer);
}

bool aes_key_load_path(const char *path) {
  const int fd = open(path, O_RDONLY);
  if (fd == -1) {
    tvkprintf(net_crypto_aes, 1, "Cannot open() AES key path: \"%s\": %m\n", path);
    return false;
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    close(fd);
    tvkprintf(net_crypto_aes, 1, "Cannot fstat() AES key path fd: %d: %m\n", fd);
    return false;
  }

  if (S_ISREG(st.st_mode)) {
    return aes_key_load_file(fd, path);
  }

  if (S_ISDIR(st.st_mode)) {
    return aes_key_load_dir(fd);
  }

  close(fd);
  tvkprintf(net_crypto_aes, 1, "Unexpected file type for AES key path: %u\n", S_IFMT & st.st_mode);

  return false;
}

aes_key_t *find_aes_key_by_id(int32_t id) {
  for (size_t i = 0; i < aes_loaded_keys_size; ++i) {
    aes_key_t *key = aes_loaded_keys[i];
    if (key->id == id) {
      return key;
    }
  }

  return NULL;
}

aes_key_t *find_aes_key_by_filename(const char *filename) {
  for (size_t i = 0; i < aes_loaded_keys_size; ++i) {
    aes_key_t *key = aes_loaded_keys[i];
    if (!strcmp(key->filename, filename)) {
      return key;
    }
  }

  return NULL;
}
