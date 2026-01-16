// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VK_NET_CRYPTO_AES_H__
#define	__VK_NET_CRYPTO_AES_H__

#include <stdlib.h>

#include "openssl/aes.h"

#include "common/pid.h"
#include "common/crypto/aes256.h"

#include "net/net-aes-keys.h"
#include "net/net-connections.h"
#include "net/net-sockaddr-storage.h"

int aes_crypto_init (struct connection *c, void *key_data, int key_data_len);  /* < 0 = error */
int aes_crypto_free (struct connection *c);
int aes_crypto_encrypt_output (struct connection *c);  /* 0 = all ok, >0 = so much more bytes needed to encrypt last block */
int aes_crypto_decrypt_input (struct connection *c);   /* 0 = all ok, >0 = so much more bytes needed to decrypt last block */
int aes_crypto_needed_output_bytes (struct connection *c);	/* returns # of bytes needed to complete last output block */

/* for aes_crypto_init */
struct aes_session_key {
  unsigned char read_key[32];
  unsigned char read_iv[16];
  unsigned char write_key[32];
  unsigned char write_iv[16];
  int32_t key_id;
};
typedef struct aes_session_key aes_session_key_t;

struct aes_udp_session_key {
  unsigned char read_key[32];
  unsigned char write_key[32];
  int32_t key_id;
};
typedef struct aes_udp_session_key aes_udp_session_key_t;

/* for c->crypto */
struct aes_crypto {
  unsigned char read_iv[16], write_iv[16];
  /*  AES_KEY read_aeskey;
      AES_KEY write_aeskey; */
  vk_aes_ctx_t read_aeskey;
  vk_aes_ctx_t write_aeskey;
};

extern int allocated_aes_crypto;
extern int aes_initialized;

int aes_load_keys();
int aes_generate_nonce (char res[16]);

int aes_create_keys(aes_key_t *key, struct aes_session_key *R, int am_client, const char nonce_server[16], const char nonce_client[16], int client_timestamp,
                    unsigned server_ip, unsigned short server_port, const unsigned char server_ipv6[16], unsigned client_ip, unsigned short client_port,
                    const unsigned char client_ipv6[16], bool is_ip_v6);
int aes_create_udp_keys (const aes_key_t* key, aes_udp_session_key_t *R, const process_id_t *local_pid, const process_id_t *remote_pid, int generation);

static inline int aes_create_connection_keys(unsigned char protocol_version, aes_key_t *key, struct aes_session_key *R, int am_client, char nonce_server[16], char nonce_client[16],
                                             int server_timestamp, int client_timestamp, struct connection *c) {
  uint32_t our_ip, remote_ip;
  uint16_t our_port, remote_port;
  uint8_t remote_ipv6[16] = {'\0'};
  uint8_t our_ipv6[16] = {'\0'};
  our_ip = remote_ip = our_port = remote_port = 0;

  bool is_ipv6 = false;

  if (protocol_version == 0) {
    assert(c->local_endpoint.ss_family == c->remote_endpoint.ss_family);
    switch (c->local_endpoint.ss_family) {
    case AF_INET:
      our_ip = inet_sockaddr_address(&c->local_endpoint);
      remote_ip = inet_sockaddr_address(&c->remote_endpoint);
      our_port = inet_sockaddr_port(&c->local_endpoint);
      remote_port = inet_sockaddr_port(&c->remote_endpoint);
      break;
    case AF_INET6:
      memcpy(remote_ipv6, inet6_sockaddr_address(&c->remote_endpoint), sizeof(remote_ipv6));
      memcpy(our_ipv6, inet6_sockaddr_address(&c->local_endpoint), sizeof(our_ipv6));
      our_port = inet6_sockaddr_port(&c->local_endpoint);
      remote_port = inet6_sockaddr_port(&c->remote_endpoint);
      is_ipv6 = true;
      break;
    default:
      assert(0);
    }
  }

  return am_client
           ? aes_create_keys(key, R, am_client, nonce_server, nonce_client, client_timestamp, protocol_version == 0 ? remote_ip : server_timestamp, remote_port, remote_ipv6, our_ip, our_port, our_ipv6, is_ipv6)
           : aes_create_keys(key, R, am_client, nonce_server, nonce_client, client_timestamp, protocol_version == 0 ? our_ip : server_timestamp, our_port, our_ipv6, remote_ip, remote_port, remote_ipv6, is_ipv6);
}

static inline int get_crypto_key_id(const aes_key_t *key) {
  assert(key);

  return key->id;
}

#endif
