// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_RPC_INCLUDE_H__
#define __VKEXT_RPC_INCLUDE_H__

#include "common/tl/constants/common.h"
#include "common/rpc-headers.h"

#include "vkext/vkext-rpc.h"

//#define DEBUG_MALLOC_LINES
//#define DEBUG_EMALLOC_LINES

#define UNUSED __attribute__ ((unused))
extern struct rpc_buffer *outbuf;
extern struct rpc_buffer *inbuf;

//#define STORE_DEBUG

/* memory {{{ */

#ifdef DEBUG_MALLOC_LINES
static inline void *zzmalloc_(long long x, char *where, int line) UNUSED;
static inline void *zzmalloc_(long long x, char *where, int line) {
#else
static inline void *zzmalloc(long long x) UNUSED;
static inline void *zzmalloc(long long x) {
#endif
  ADD_CNT (malloc);
  START_TIMER (malloc);
  void *r = malloc(x);
#ifdef DEBUG_MALLOC_LINES
  fprintf(stderr, "allocating %lld bytes at %s:%d p = %p\n", x, where, line, r);
#endif
  assert (r);
  ADD_MALLOC (x);
  END_TIMER (malloc);
  return r;
}

#ifdef DEBUG_MALLOC_LINES
static inline void *zzmalloc0_(long long x, char *where, int line) UNUSED;
static inline void *zzmalloc0_(long long x, char *where, int line) {
#else
static inline void *zzmalloc0(long long x) UNUSED;
static inline void *zzmalloc0(long long x) {
#endif
  ADD_CNT (malloc);
  START_TIMER (malloc);
  void *r = malloc(x);
#ifdef DEBUG_MALLOC_LINES
  fprintf(stderr, "allocating %lld zeroed bytes at %s:%d, p = %p\n", x, where, line, r);
#endif
  assert (r);
  memset(r, 0, x);
  ADD_MALLOC (x);
  END_TIMER (malloc);
  return r;
}

#ifdef DEBUG_MALLOC_LINES
#define zzmalloc(x) zzmalloc_ (x, __FILE__,  __LINE__)
#define zzmalloc0(x) zzmalloc0_ (x, __FILE__, __LINE__)
#endif

#ifdef DEBUG_EMALLOC_LINES
static inline void *zzemalloc_(long long x, char *where, int line) UNUSED;
static inline void *zzemalloc_(long long x, char *where, int line) {
#else
static inline void *zzemalloc(long long x) UNUSED;
static inline void *zzemalloc(long long x) {
#endif
  ADD_CNT (emalloc);
  START_TIMER (emalloc);
  void *r = emalloc (x);
#ifdef DEBUG_EMALLOC_LINES
  fprintf(stderr, "[e] allocating %lld bytes at %s:%d, p = %p\n", x, where, line, r);
#endif
  END_TIMER (emalloc);
  assert (r);
  ADD_EMALLOC (x);
  return r;
}

#ifdef DEBUG_EMALLOC_LINES
#define zzemalloc(x) zzemalloc_ (x, __FILE__,  __LINE__)
#endif


static inline void *zzrealloc(void *p, long long x, long long y) UNUSED;
static inline void *zzrealloc(void *p, long long x, long long y) {
  void *r = realloc(p, y);
  assert (r);
  ADD_REALLOC (x, y);
  return r;
}

static inline void *zzerealloc(void *p, long long x, long long y) UNUSED;
static inline void *zzerealloc(void *p, long long x, long long y) {
  void *r = erealloc (p, y);
  assert (r);
  ADD_EREALLOC (x, y);
  return r;
}

#ifdef DEBUG_MALLOC_LINES
static inline void zzfree_(void *p, long long x, char *where, int line) UNUSED;
static inline void zzfree_(void *p, long long x, char *where, int line) {
#else
static inline void zzfree(void *p, long long x) UNUSED;
static inline void zzfree(void *p, long long x) {
#endif
  ADD_CNT (malloc);
  START_TIMER (malloc);
#ifdef DEBUG_MALLOC_LINES
  fprintf(stderr, "free %lld bytes at %s:%d , p = %p\n", x, where, line, p);
#endif
  free(p);
  END_TIMER (malloc);
  ADD_FREE (x);
}

#ifdef DEBUG_MALLOC_LINES
#define zzfree(p, x) zzfree_(p, x, __FILE__, __LINE__)
#endif

#ifdef DEBUG_EMALLOC_LINES
static inline void zzefree_(void *p, long long x, char *where, int line) UNUSED;
static inline void zzefree_(void *p, long long x, char *where, int line) {
#else
static inline void zzefree(void *p, long long x) UNUSED;
static inline void zzefree(void *p, long long x) {
#endif
  ADD_CNT (emalloc);
  START_TIMER (emalloc);
#ifdef DEBUG_EMALLOC_LINES
  fprintf(stderr, "[e] free %lld bytes at %s:%d , p = %p\n", x, where, line, p);
#endif
  efree (p);
  END_TIMER (emalloc);
  ADD_EFREE (x);
}

#ifdef DEBUG_EMALLOC_LINES
#define zzefree(p, x) zzefree_(p, x, __FILE__, __LINE__)
#endif


static inline char *zzstrdup(const char *p) {
  if (!p) {
    return 0;
  }
  ADD_MALLOC (strlen(p));
  return strdup(p);
}

static inline void zzstrfree(char *p) {
  if (!p) {
    return;
  }
  ADD_FREE (strlen(p));
  free(p);
}
/* }}} */

/* outbuf {{{ */

static struct rpc_buffer *buffer_delete(struct rpc_buffer *buf) UNUSED;
static struct rpc_buffer *buffer_delete(struct rpc_buffer *buf) {
  buf->magic = 0;
  zzefree(buf->sptr, buf->eptr - buf->sptr);
  zzfree(buf, sizeof(*buf));
  return 0;
}

static struct rpc_buffer *buffer_create(int start_len) UNUSED;
static struct rpc_buffer *buffer_create(int start_len) {
  rpc_buffer *buf = reinterpret_cast<rpc_buffer *>(zzmalloc(sizeof(struct rpc_buffer)));
  buf->magic = RPC_BUFFER_MAGIC;
  buf->sptr = static_cast<char *>(zzemalloc(start_len));
  buf->eptr = buf->sptr + start_len;
  buf->wptr = buf->rptr = buf->sptr;
  return buf;
}

static inline void buffer_clear(struct rpc_buffer *buf) UNUSED;
static inline void buffer_clear(struct rpc_buffer *buf) {
  buf->rptr = buf->wptr = buf->sptr;
}

static inline struct rpc_buffer *buffer_create_data(void *data, int len) UNUSED;
static inline struct rpc_buffer *buffer_create_data(void *data, int len) {
  rpc_buffer *buf = reinterpret_cast<rpc_buffer *>(zzmalloc(sizeof(rpc_buffer)));
  buf->magic = RPC_BUFFER_MAGIC;
  buf->rptr = buf->sptr = static_cast<char *>(data);
  buf->wptr = buf->eptr = buf->sptr + len;
  return buf;
}

static inline void buffer_check_len_wptr(struct rpc_buffer *buf, int x) UNUSED;
static inline void buffer_check_len_wptr(struct rpc_buffer *buf, int x) {
  if (buf->wptr + x > buf->eptr) {
    int new_len = 2 * (buf->eptr - buf->sptr + 100);
    if (new_len < x + (buf->wptr - buf->sptr)) {
      new_len = x + (buf->wptr - buf->sptr) + ((-x) & 3);
    }
    char *t = static_cast<char *>(zzerealloc(buf->sptr, buf->eptr - buf->sptr, new_len));
    ADD_CNT (realloc);
    assert (t);
    buf->wptr = t + (buf->wptr - buf->sptr);
    buf->rptr = t + (buf->rptr - buf->sptr);
    buf->eptr = t + new_len;
    buf->sptr = t;
  }
}

static inline int buffer_check_len_rptr(struct rpc_buffer *buf, int x) UNUSED;
static inline int buffer_check_len_rptr(struct rpc_buffer *buf, int x) {
  return (buf->rptr + x <= buf->wptr);
}

static inline void buffer_write_char(struct rpc_buffer *buf, char x) UNUSED;
static inline void buffer_write_char(struct rpc_buffer *buf, char x) {
  buffer_check_len_wptr(buf, 1);
  *(char *)(buf->wptr) = x;
  buf->wptr++;
}

static inline void buffer_write_int(struct rpc_buffer *buf, int x) UNUSED;
static inline void buffer_write_int(struct rpc_buffer *buf, int x) {
  buffer_check_len_wptr(buf, 4);
  *(int *)(buf->wptr) = x;
  buf->wptr += 4;
}

static inline void buffer_write_long(struct rpc_buffer *buf, long long x) UNUSED;
static inline void buffer_write_long(struct rpc_buffer *buf, long long x) {
  buffer_check_len_wptr(buf, 8);
  *(long long *)(buf->wptr) = x;
  buf->wptr += 8;
}

static inline void buffer_write_double(struct rpc_buffer *buf, double x) UNUSED;
static inline void buffer_write_double(struct rpc_buffer *buf, double x) {
  buffer_check_len_wptr(buf, sizeof(x));
  *(double *)(buf->wptr) = x;
  buf->wptr += sizeof(x);
}

static inline void buffer_write_float(struct rpc_buffer *buf, float x) UNUSED;
static inline void buffer_write_float(struct rpc_buffer *buf, float x) {
  buffer_check_len_wptr(buf, sizeof(x));
  *(float *)(buf->wptr) = x;
  buf->wptr += sizeof(x);
}

static inline void buffer_write_data(struct rpc_buffer *buf, const void *x, int len) UNUSED;
static inline void buffer_write_data(struct rpc_buffer *buf, const void *x, int len) {
  buffer_check_len_wptr(buf, len);
  memcpy(buf->wptr, x, len);
  buf->wptr += len;
}

static inline void buffer_write_pad(struct rpc_buffer *buf) UNUSED;
static inline void buffer_write_pad(struct rpc_buffer *buf) {
  int pad_bytes = (int)((buf->sptr - buf->wptr) & 3);
  buffer_check_len_wptr(buf, pad_bytes);
  memset(buf->wptr, 0, pad_bytes);
  buf->wptr += pad_bytes;
}

static inline void buffer_write_string(struct rpc_buffer *buf, int len, const char *s) UNUSED;
static inline void buffer_write_string(struct rpc_buffer *buf, int len, const char *s) {
  assert (!(len & 0xff000000));
  buffer_write_char(buf, static_cast<char>(254));
  buffer_write_data(buf, &len, 3);
  buffer_write_data(buf, s, len);
  buffer_write_pad(buf);
}

static inline void buffer_write_string_tiny(struct rpc_buffer *buf, int len, const char *s) UNUSED;
static inline void buffer_write_string_tiny(struct rpc_buffer *buf, int len, const char *s) {
  assert (len <= 253);
  buffer_write_char(buf, len);
  buffer_write_data(buf, s, len);
  buffer_write_pad(buf);
}

static inline void buffer_write_reserve(struct rpc_buffer *buf, int len) UNUSED;
static inline void buffer_write_reserve(struct rpc_buffer *buf, int len) {
  assert (buf->rptr == buf->wptr);
  buffer_check_len_wptr(buf, len);
  buf->wptr += len;
  buf->rptr += len;
}

static inline int buffer_read_char(struct rpc_buffer *buf, char *x) UNUSED;
static inline int buffer_read_char(struct rpc_buffer *buf, char *x) {
  if (!buffer_check_len_rptr(buf, 1)) {
    return -1;
  } else {
    *x = *(char *)buf->rptr;
    buf->rptr += 1;
    return 1;
  }
}

static inline int buffer_read_int(struct rpc_buffer *buf, int *x) UNUSED;
static inline int buffer_read_int(struct rpc_buffer *buf, int *x) {
  if (!buffer_check_len_rptr(buf, 4)) {
    return -1;
  } else {
    *x = *(int *)buf->rptr;
    buf->rptr += 4;
    return 1;
  }
}

static inline int buffer_read_long(struct rpc_buffer *buf, long long *x) UNUSED;
static inline int buffer_read_long(struct rpc_buffer *buf, long long *x) {
  if (!buffer_check_len_rptr(buf, 8)) {
    return -1;
  } else {
    *x = *(long long *)buf->rptr;
    buf->rptr += 8;
    return 1;
  }
}

static inline int buffer_read_double(struct rpc_buffer *buf, double *x) UNUSED;
static inline int buffer_read_double(struct rpc_buffer *buf, double *x) {
  if (!buffer_check_len_rptr(buf, sizeof(*x))) {
    return -1;
  } else {
    *x = *(double *)buf->rptr;
    buf->rptr += sizeof(*x);
    return 1;
  }
}

static inline int buffer_read_float(struct rpc_buffer *buf, float *x) UNUSED;
static inline int buffer_read_float(struct rpc_buffer *buf, float *x) {
  if (!buffer_check_len_rptr(buf, sizeof(*x))) {
    return -1;
  } else {
    *x = *(float *)buf->rptr;
    buf->rptr += sizeof(*x);
    return 1;
  }
}

static inline int buffer_read_data(struct rpc_buffer *buf, int len, const char **x) UNUSED;
static inline int buffer_read_data(struct rpc_buffer *buf, int len, const char **x) {
  if (!buffer_check_len_rptr(buf, len)) {
    return -1;
  } else {
    *x = buf->rptr;
    buf->rptr += len;
    //buf->rptr += len + ((-len) & 3);
    return 1;
  }
}

static inline int buffer_read_pad(struct rpc_buffer *buf) UNUSED;
static inline int buffer_read_pad(struct rpc_buffer *buf) {
  int pad_bytes = (buf->sptr - buf->rptr) & 3;
  buf->rptr += pad_bytes;
  return 1;
}

static inline int buffer_read_string(struct rpc_buffer *buf, int *len, const char **x) UNUSED;
static inline int buffer_read_string(struct rpc_buffer *buf, int *len, const char **x) {
  unsigned char c;
  if (buffer_read_char(buf, (char *)&c) < 0) {
    return -1;
  }
  *len = c;
  if (*len == 255) {
    return -1;
  }
  if (*len == 254) {
    const char *t;
    *len = 0;
    if (buffer_read_data(buf, 3, &t) < 0) {
      return -1;
    }
    memcpy(len, t, 3);
  }
  if (buffer_read_data(buf, *len, x) < 0) {
    return -1;
  }
  buffer_read_pad(buf);
  return 1;
}

/* }}} outbuf */

static constexpr size_t RPC_HEADERS_RESERVED_BYTES = 40;
static_assert(RPC_HEADERS_RESERVED_BYTES >= sizeof(RpcHeaders) + sizeof(RpcExtraHeaders), "Not enough reserved bytes");

static void do_rpc_clean() UNUSED;
static void do_rpc_clean() { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  if (outbuf) {
    buffer_clear(outbuf);
  } else {
    outbuf = buffer_create(0);
  }
  buffer_write_reserve(outbuf, RPC_HEADERS_RESERVED_BYTES);
  END_TIMER (store);
}
/* }}} */

static void do_rpc_store_int(int value) UNUSED;
static void do_rpc_store_int(int value) { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  buffer_write_int(outbuf, value);
#ifdef STORE_DEBUG
  fprintf (stderr, "int: %x\n", value);
#endif
  END_TIMER (store);
}
/* }}} */

static int do_rpc_store_get_pos() UNUSED;
static int do_rpc_store_get_pos() { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  END_TIMER (store);
  return outbuf->wptr - outbuf->rptr;
}
/* }}} */

static int do_rpc_store_set_pos(int p) UNUSED;
static int do_rpc_store_set_pos(int p) { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  if (p < 0 || outbuf->rptr + p > outbuf->eptr) {
    return 0;
  }
  outbuf->wptr = outbuf->rptr + p;
  END_TIMER (store);
  return 1;
}
/* }}} */

static void do_rpc_store_long(long long value) UNUSED;
static void do_rpc_store_long(long long value) { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  buffer_write_long(outbuf, value);
#ifdef STORE_DEBUG
  fprintf (stderr, "long: %lld\n", value);
#endif
  END_TIMER (store);
}
/* }}} */

static void do_rpc_store_double(double value) UNUSED;
static void do_rpc_store_double(double value) { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  buffer_write_double(outbuf, value);
#ifdef STORE_DEBUG
  fprintf (stderr, "double: %lf\n", value);
#endif
  END_TIMER (store);
}
/* }}} */

static void do_rpc_store_float(float value) UNUSED;
static void do_rpc_store_float(float value) { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  buffer_write_float(outbuf, value);
#ifdef STORE_DEBUG
  fprintf (stderr, "float: %lf\n", value);
#endif
  END_TIMER (store);
}
/* }}} */

static void do_rpc_store_string(const char *s, int len) UNUSED;
static void do_rpc_store_string(const char *s, int len) { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (len <= 0xffffff);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  if (len <= 253) {
    buffer_write_string_tiny(outbuf, len, s);
  } else {
    buffer_write_string(outbuf, len, s);
  }
#ifdef STORE_DEBUG
  fprintf (stderr, "string: %.*s\n", len, s);
#endif
  END_TIMER (store);
}
/* }}} */

static void do_rpc_store_header(long long cluster_id, int flags) UNUSED;
static void do_rpc_store_header(long long cluster_id, int flags) { /* {{{ */
  ADD_CNT (store);
  START_TIMER (store);
  assert (outbuf && outbuf->magic == RPC_BUFFER_MAGIC);
  if (flags) {
    buffer_write_int(outbuf, TL_RPC_DEST_ACTOR_FLAGS);
    buffer_write_long(outbuf, cluster_id);
    buffer_write_int(outbuf, flags);
  } else {
    buffer_write_int(outbuf, TL_RPC_DEST_ACTOR);
    buffer_write_long(outbuf, cluster_id);
  }
  END_TIMER (store);
}
/* }}} */

#endif
