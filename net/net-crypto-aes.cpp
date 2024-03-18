// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-crypto-aes.h"

#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/cycleclock.h"
#include "common/kprintf.h"
#include "common/md5.h"
#include "common/options.h"
#include "common/precise-time.h"
#include "common/secure-bzero.h"
#include "common/server/signals.h"
#include "common/sha1.h"
#include "common/wrappers/memory-utils.h"

#include "net/net-aes-keys.h"
#include "net/net-connections.h"

int allocated_aes_crypto;

static char rand_buf[64];

int aes_crypto_init(struct connection *c, void *key_data, int key_data_len) {
  assert(key_data_len == sizeof(struct aes_session_key));
  struct aes_crypto *T = static_cast<aes_crypto *>(malloc(sizeof(struct aes_crypto)));
  struct aes_session_key *D = static_cast<aes_session_key *>(key_data);
  assert(T);
  ++allocated_aes_crypto;
  /* AES_set_decrypt_key (D->read_key, 256, &T->read_aeskey); */
  vk_aes_set_decrypt_key(&T->read_aeskey, D->read_key, 256);
  memcpy(T->read_iv, D->read_iv, 16);
  /* AES_set_encrypt_key (D->write_key, 256, &T->write_aeskey); */
  vk_aes_set_encrypt_key(&T->write_aeskey, D->write_key, 256);
  memcpy(T->write_iv, D->write_iv, 16);
  c->crypto = T;
  return 0;
}

int aes_crypto_free(struct connection *c) {
  if (c->crypto) {
    free(c->crypto);
    c->crypto = 0;
    --allocated_aes_crypto;
  }
  return 0;
}

/* 0 = all ok, >0 = so much more bytes needed to encrypt last block */
int aes_crypto_encrypt_output(struct connection *c) {
  static nb_processor_t P;
  struct aes_crypto *T = static_cast<aes_crypto *>(c->crypto);
  assert(c->crypto);

  // dump_buffers (&c->Out);

  nb_start_process(&P, &c->Out);
  while (P.len0 + P.len1 >= 16) {
    assert(P.len0 >= 0 && P.len1 >= 0);
    if (P.len0 >= 16) {
      // AES_cbc_encrypt ((unsigned char *)P.ptr0, (unsigned char *)P.ptr0, P.len0 & -16, &T->write_aeskey, T->write_iv, AES_ENCRYPT);
      T->write_aeskey.cbc_crypt(&T->write_aeskey, (unsigned char *)P.ptr0, (unsigned char *)P.ptr0, P.len0 & -16, T->write_iv);
      nb_advance_process(&P, P.len0 & -16);
    } else {
      static unsigned char tmpbuf[16];
      memcpy(tmpbuf, P.ptr0, P.len0);
      memcpy(tmpbuf + P.len0, P.ptr1, 16 - P.len0);
      // AES_cbc_encrypt (tmpbuf, tmpbuf, 16, &T->write_aeskey, T->write_iv, AES_ENCRYPT);
      T->write_aeskey.cbc_crypt(&T->write_aeskey, tmpbuf, tmpbuf, 16, T->write_iv);
      memcpy(P.ptr0, tmpbuf, P.len0);
      memcpy(P.ptr1, tmpbuf + P.len0, 16 - P.len0);
      nb_advance_process(&P, 16);
    }
  }

  // dump_buffers (&c->Out);

  assert(P.len0 + P.len1 == c->Out.unprocessed_bytes);

  return c->Out.unprocessed_bytes ? 16 - c->Out.unprocessed_bytes : 0;
}

/* 0 = all ok, >0 = so much more bytes needed to decrypt last block */
int aes_crypto_decrypt_input(struct connection *c) {
  static nb_processor_t P;
  struct aes_crypto *T = static_cast<aes_crypto *>(c->crypto);
  assert(c->crypto);

  // dump_buffers (&c->In);

  nb_start_process(&P, &c->In);
  while (P.len0 + P.len1 >= 16) {
    assert(P.len0 >= 0 && P.len1 >= 0);
    if (P.len0 >= 16) {
      // AES_cbc_encrypt ((unsigned char *)P.ptr0, (unsigned char *)P.ptr0, P.len0 & -16, &T->read_aeskey, T->read_iv, AES_DECRYPT);
      T->read_aeskey.cbc_crypt(&T->read_aeskey, (unsigned char *)P.ptr0, (unsigned char *)P.ptr0, P.len0 & -16, T->read_iv);
      nb_advance_process(&P, P.len0 & -16);
    } else {
      static unsigned char tmpbuf[16];
      memcpy(tmpbuf, P.ptr0, P.len0);
      memcpy(tmpbuf + P.len0, P.ptr1, 16 - P.len0);
      // AES_cbc_encrypt (tmpbuf, tmpbuf, 16, &T->read_aeskey, T->read_iv, AES_DECRYPT);
      T->read_aeskey.cbc_crypt(&T->read_aeskey, tmpbuf, tmpbuf, 16, T->read_iv);
      memcpy(P.ptr0, tmpbuf, P.len0);
      memcpy(P.ptr1, tmpbuf + P.len0, 16 - P.len0);
      nb_advance_process(&P, 16);
    }
  }

  assert(P.len0 + P.len1 == c->In.unprocessed_bytes);

  // dump_buffers (&c->In);

  return c->In.unprocessed_bytes ? 16 - c->In.unprocessed_bytes : 0;
}

/* returns # of bytes needed to complete last output block */
int aes_crypto_needed_output_bytes(struct connection *c) {
  assert(c->crypto);
  return -c->Out.unprocessed_bytes & 15;
}

int aes_initialized;

static int initialize_pseudo_random() {
  int fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
  int r = 0;

  if (fd >= 0) {
    r = read(fd, rand_buf, 16);
    if (r > 0) {
      tvkprintf(net_crypto_aes, 4, "added %d bytes of real entropy to the seed\n", r);
    }
    close(fd);

    r = r < 0 ? 0 : r;
  }

  if (r < 16) {
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
      return -1;
    }
    const int s = read(fd, rand_buf + r, 16 - r);
    if (r + s != 16) {
      close(fd);
      return -1;
    }
    close(fd);
  }

  *(long *)rand_buf ^= lrand48();
  srand48(*(long *)rand_buf);

  return 1;
}

static const char *aes_pwd_path = NULL;

SAVE_STRING_OPTION_PARSER(OPT_NETWORK, "aes-pwd", aes_pwd_path, "sets pwd file");

int aes_load_keys() {
  if (initialize_pseudo_random() < 0) {
    assert(!default_aes_key);
    return -1;
  }

  if (aes_key_load_path(aes_pwd_path ? aes_pwd_path : "/etc/engine/pass")) {
    aes_initialized = 1;
    return 1;
  }
  return -1;
}

int aes_generate_nonce(char res[16]) {
  *(int *)(rand_buf + 16) = lrand48();
  *(int *)(rand_buf + 20) = lrand48();
  *(long long *)(rand_buf + 24) = cycleclock_now();
  struct timespec T;
  assert(clock_gettime(CLOCK_REALTIME, &T) >= 0);
  *(int *)(rand_buf + 32) = T.tv_sec;
  *(int *)(rand_buf + 36) = T.tv_nsec;
  (*(int *)(rand_buf + 40))++;

  md5((unsigned char *)rand_buf, 44, (unsigned char *)res);
  return 0;
}

// str :=
// nonce_server.nonce_client.client_timestamp.server_ip.client_port.("SERVER"/"CLIENT").client_ip.server_port.master_key.nonce_server.[client_ipv6.server_ipv6].nonce_client
// key := SUBSTR(MD5(str+1),0,12).SHA1(str)
// iv  := MD5(str+2)

int aes_create_keys(aes_key_t *key, struct aes_session_key *R, int am_client, const char nonce_server[16], const char nonce_client[16], int client_timestamp,
                    unsigned server_ip, unsigned short server_port, const unsigned char server_ipv6[16], unsigned client_ip, unsigned short client_port,
                    const unsigned char client_ipv6[16]) {
  unsigned char str[16 + 16 + 4 + 4 + 2 + 6 + 4 + 2 + AES_KEY_MAX_LEN + 16 + 16 + 4 + 16 * 2];
  int str_len;

  if (!key) {
    return -1;
  }

  const int pwd_len = key->len;
  char *pwd_buf = (char *)key->key;

  memcpy(str, nonce_server, 16);
  memcpy(str + 16, nonce_client, 16);
  *((int *)(str + 32)) = client_timestamp;
  *((unsigned *)(str + 36)) = server_ip;
  *((unsigned short *)(str + 40)) = client_port;
  memcpy(str + 42, am_client ? "CLIENT" : "SERVER", 6);
  *((unsigned *)(str + 48)) = client_ip;
  *((unsigned short *)(str + 52)) = server_port;
  memcpy(str + 54, pwd_buf, pwd_len);
  memcpy(str + 54 + pwd_len, nonce_server, 16);
  str_len = 70 + pwd_len;

  if (!server_ip) {
    assert(!client_ip);
    memcpy(str + str_len, client_ipv6, 16);
    memcpy(str + str_len + 16, server_ipv6, 16);
    str_len += 32;
  } else {
    assert(client_ip);
  }

  memcpy(str + str_len, nonce_client, 16);
  str_len += 16;

  md5(str + 1, str_len - 1, R->write_key);
  sha1(str, str_len, R->write_key + 12);
  md5(str + 2, str_len - 2, R->write_iv);

  memcpy(str + 42, !am_client ? "CLIENT" : "SERVER", 6);

  md5(str + 1, str_len - 1, R->read_key);
  sha1(str, str_len, R->read_key + 12);
  md5(str + 2, str_len - 2, R->read_iv);

  secure_bzero(str, str_len);

  R->key_id = key->id;

  return 1;
}

// str := server_pid . key . client_pid
// key := SUBSTR(MD5(str+1),0,12).SHA1(str)
// iv  := MD5(str+2)

int aes_create_udp_keys(const aes_key_t *key, aes_udp_session_key_t *R, const process_id_t *local_pid, const process_id_t *remote_pid, int generation) {
  unsigned char str[16 + 16 + 4 + 4 + 2 + 6 + 4 + 2 + AES_KEY_MAX_LEN + 16 + 16 + 4 + 16 * 2];
  int str_len;

  if (!key) {
    return -1;
  }

  const int pwd_len = key->len;
  char *pwd_buf = (char *)key->key;

  memcpy(str, local_pid, 12);
  memcpy(str + 12, pwd_buf, pwd_len);
  memcpy(str + 12 + pwd_len, remote_pid, 12);
  memcpy(str + 24 + pwd_len, &generation, 4);
  str_len = 28 + pwd_len;

  md5(str + 1, str_len - 1, R->write_key);
  sha1(str, str_len, R->write_key + 12);

  memcpy(str, remote_pid, 12);
  memcpy(str + 12 + pwd_len, local_pid, 12);

  md5(str + 1, str_len - 1, R->read_key);
  sha1(str, str_len, R->read_key + 12);

  secure_bzero(str, str_len);

  R->key_id = key->id;

  return 1;
}
