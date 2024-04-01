// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>
#include <stdint.h>
#include <stdlib.h>
#include <sys/uio.h>

#include "common/crc32.h"
#include "common/crypto/aes256.h"

#include "net/net-msg-part.h"

void init_msg();
int rwm_total_msgs();

/*
 *	RAW MESSAGES (struct raw_message) = chains of MESSAGE PARTs
 */

// ordinary raw message (changing refcnt of pointed msg_parts)
#define RM_INIT_MAGIC 0x23513473

#define RM_PREPEND_RESERVE 128

typedef struct raw_message {
  msg_part_t *first, *last; // 'last' doesn't increase refcnt of pointed msg_part
  int total_bytes;          // bytes in the chain (extra bytes ignored even if present)
  int magic;                // one of RM_INIT_MAGIC
  int first_offset;         // offset of first used byte inside first buffer data
  int last_offset;          // offset after last used byte inside last buffer data
} raw_message_t;

/* NB: struct raw_message itself is never allocated or freed by the following functions since
  it is usually part (field) of a larger structure
*/

static inline int msg_part_first_offset(const raw_message_t *rwm, const msg_part_t *mp) {
  return mp == rwm->first ? rwm->first_offset : mp->offset;
}

static inline int msg_part_last_offset(const raw_message_t *rwm, const msg_part_t *mp) {
  return mp == rwm->last ? rwm->last_offset : mp->offset + mp->len;
}

void rwm_free(raw_message_t *raw);
void rwm_init(raw_message_t *raw, int alloc_bytes);
int rwm_create(raw_message_t *raw, const void *data, int alloc_bytes);
void rwm_clone(raw_message_t *dest_raw, const raw_message_t *src_raw);
int rwm_push_data(raw_message_t *raw, const void *data, int alloc_bytes);
int rwm_push_data_ext(raw_message_t *raw, const void *data, int alloc_bytes, int prepend, int small_buffer, int std_buffer);
int rwm_push_data_front(raw_message_t *raw, const void *data, int alloc_bytes);
int rwm_fetch_data(raw_message_t *raw, void *data, int bytes);
int rwm_fetch_lookup(const raw_message_t *raw, void *buf, int bytes);
int rwm_fetch_data_back(raw_message_t *raw, void *data, int bytes);
int rwm_trunc(raw_message_t *raw, int len);
int rwm_union(raw_message_t *raw, raw_message_t *tail);
void rwm_union_unique(raw_message_t *head, raw_message_t *tail);
int rwm_split(raw_message_t *raw, raw_message_t *tail, int bytes);
int rwm_split_head(raw_message_t *head, raw_message_t *raw, int bytes);
void *rwm_prepend_alloc(raw_message_t *raw, int alloc_bytes);
void *rwm_postpone_alloc_ext(raw_message_t *raw, int min_alloc_bytes, int max_alloc_bytes, int prepend_reserve, int continue_buffer, int small_buffer,
                             int std_buffer, int *allocated);
void *rwm_postpone_alloc(raw_message_t *raw, int alloc_bytes);

void rwm_steal(raw_message_t *dst, raw_message_t *src);
void rwm_clean(raw_message_t *raw);
void rwm_clear(raw_message_t *raw);
void rwm_check(raw_message_t *raw);
void fork_message_chain(raw_message_t *raw);
void rwm_fork_deep(raw_message_t *raw);

int rwm_prepare_iovec(const raw_message_t *raw, struct iovec *iov, int iov_len, int bytes);
void rwm_dump_sizes(raw_message_t *raw);
void rwm_dump(raw_message_t *raw);
uint32_t rwm_crc32c(const raw_message_t *raw, int bytes);
uint32_t rwm_crc32(const raw_message_t *raw, int bytes);
uint32_t rwm_custom_crc32(const raw_message_t *raw, int bytes, crc32_partial_func_t custom_crc32_partial);

/* negative exit code of process stops processing */
typedef int (*rwm_process_callback_t)(void *extra, const void *data, int len);
typedef int (*rwm_transform_callback_t)(void *extra, void *data, int len);

int rwm_process(const raw_message_t *raw, int bytes, rwm_process_callback_t cb, void *extra);
int rwm_process_from_offset(const raw_message_t *raw, int bytes, int offset, rwm_process_callback_t cb, void *extra);
/* warning: in current realization refcnt of message chain should be 1 */
int rwm_transform_from_offset(raw_message_t *raw, int bytes, int offset, rwm_transform_callback_t cb, void *extra);
int rwm_process_and_advance(raw_message_t *raw, int bytes, rwm_process_callback_t cb, void *extra);
int rwm_encrypt_decrypt(raw_message_t *raw, int bytes, struct vk_aes_ctx *ctx, unsigned char iv[32]);
int rwm_encrypt_decrypt_cbc(raw_message_t *raw, int bytes, struct vk_aes_ctx *ctx, unsigned char iv[16]);

typedef void (*rwm_encrypt_decrypt_to_callback_t)(struct vk_aes_ctx *ctx, const void *src, void *dst, int l, unsigned char *iv);
int rwm_encrypt_decrypt_to(raw_message_t *raw, raw_message_t *res, int bytes, struct vk_aes_ctx *ctx, rwm_encrypt_decrypt_to_callback_t crypt_cb,
                           unsigned char *iv);

int rwm_process_and_advance(raw_message_t *raw, int bytes, const std::function<void(const void *, int)> &callback);
int rwm_process(const raw_message_t *raw, int bytes, const std::function<int(const void *, int)> &callback);
