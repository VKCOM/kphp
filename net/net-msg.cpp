// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-msg.h"

#include <algorithm>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include "common/crc32.h"
#include "common/crc32c.h"
#include "common/kprintf.h"
#include "common/parallel/counter.h"
#include "common/sha1.h"

#include "net/net-msg-buffers.h"

PARALLEL_COUNTER(rwm_total_msgs);

void init_msg() {
  PARALLEL_COUNTER_REGISTER_THREAD(rwm_total_msgs);
  init_msg_part();
}

int rwm_total_msgs() {
  return PARALLEL_COUNTER_READ(rwm_total_msgs);
}

// raw_message_t itself is not freed since it is usually part of a larger structure
void rwm_free(raw_message_t *raw) {
  msg_part_t *mp = raw->first;
  assert(raw->magic == RM_INIT_MAGIC);
  PARALLEL_COUNTER_DEC(rwm_total_msgs);
  memset(raw, 0, sizeof(*raw));
  msg_part_chain_dec_ref(mp);
}

static inline void fixup_msg_part_to_rwm(const raw_message_t *rwm, msg_part_t *msg_part) {
  const int delta = rwm->first_offset - msg_part->offset;
  assert(delta >= 0 && msg_part->len >= delta && msg_part->len <= msg_buffer_size(msg_part->buffer));
  msg_part->offset += delta;
  msg_part->len -= delta;
}

void fork_message_chain(raw_message_t *raw) {
  assert(raw->magic == RM_INIT_MAGIC);
  msg_part_t *mp = raw->first, **mpp = &raw->first;
  int total_bytes = raw->total_bytes;
  if (!mp) {
    return;
  }
  if (raw->first_offset != mp->offset) {
    if (msg_part_use_count(mp) == 1) {
      fixup_msg_part_to_rwm(raw, mp);
    }
  }
  while (mp != raw->last && msg_part_use_count(mp) == 1) {
    total_bytes -= mp->len;
    mpp = &mp->next;
    mp = mp->next;
    assert(mp);
  }
  if (msg_part_use_count(mp) != 1) {
    bool last_copied = false;

    msg_part_t *mp_pivot = mp;
    while (!last_copied) {
      assert(mp);
      msg_part_t *mpc = new_msg_part(mp->buffer);
      msg_buffer_inc_ref(mpc->buffer);
      mpc->offset = mp->offset;
      mpc->len = mp->len;

      if (mp == raw->first && raw->first_offset != mp->offset) {
        fixup_msg_part_to_rwm(raw, mpc);
      }

      if (mp == raw->last) {
        mpc->len = raw->last_offset - mpc->offset;
        assert(mpc->len >= 0);
        raw->last = mpc;
        last_copied = true;
      }
      *mpp = mpc;
      total_bytes -= mpc->len;

      mpp = &mpc->next;
      mp = mp->next;
    }

    msg_part_chain_dec_ref(mp_pivot);
  } else {
    assert(mp == raw->last);
    if (raw->last_offset != mp->offset + mp->len) {
      mp->len = raw->last_offset - mp->offset;
      assert(mp->len >= 0);
    }
    total_bytes -= mp->len;
    msg_part_chain_dec_ref(mp->next);
    mp->next = NULL;
  }
  if (total_bytes) {
    kprintf("total_bytes = %d\n", total_bytes);
  }
  assert(!total_bytes);
}

void rwm_steal(raw_message_t *dst, raw_message_t *src) {
  *dst = *src;
  PARALLEL_COUNTER_INC(rwm_total_msgs);
  rwm_clean(src);
}

void rwm_clean(raw_message_t *raw) {
  assert(raw->magic == RM_INIT_MAGIC);
  raw->first = raw->last = NULL;
  raw->first_offset = raw->last_offset = 0;
  raw->total_bytes = 0;
}

void rwm_clear(raw_message_t *raw) {
  assert(raw->magic == RM_INIT_MAGIC);
  if (raw->first) {
    msg_part_chain_dec_ref(raw->first);
  }
  rwm_clean(raw);
}

void rwm_clone(raw_message_t *dest_raw, const raw_message_t *src_raw) {
  assert(src_raw->magic == RM_INIT_MAGIC);
  memcpy(dest_raw, src_raw, sizeof(raw_message_t));
  if (src_raw->magic == RM_INIT_MAGIC && src_raw->first) {
    msg_part_inc_ref(src_raw->first);
  }
  PARALLEL_COUNTER_INC(rwm_total_msgs);
}

int rwm_push_data_ext(raw_message_t *raw, const void *data, int alloc_bytes, int prepend, int small_buffer, int std_buffer) {
  assert(raw->magic == RM_INIT_MAGIC);
  assert(alloc_bytes >= 0);
  if (!alloc_bytes) {
    return 0;
  }
  msg_part_t *mp, *mpl;
  int res = 0;
  if (!raw->first) {
    msg_buffer_t *buffer = alloc_msg_buffer(alloc_bytes >= small_buffer - prepend ? std_buffer : small_buffer);
    if (!buffer) {
      return 0;
    }
    mp = new_msg_part(buffer);
    if (alloc_bytes <= std_buffer) {
      if (prepend > std_buffer - alloc_bytes) {
        prepend = std_buffer - alloc_bytes;
      }
    }
    mp->offset = prepend;
    const int sz = msg_buffer_size(buffer) - prepend;
    raw->first = raw->last = mp;
    raw->first_offset = prepend;
    const int to_copy = std::min(sz, alloc_bytes);
    mp->len = to_copy;
    raw->total_bytes = to_copy;
    raw->last_offset = to_copy + prepend;
    if (data) {
      memcpy(buffer->data + prepend, data, to_copy);
      data = static_cast<const char *>(data) + to_copy;
    }
    alloc_bytes -= to_copy;
    res = to_copy;
  } else {
    mp = raw->last;
    assert(mp);
    if (mp->next || raw->last_offset != mp->offset + mp->len) {
      assert(raw->last_offset <= mp->offset + mp->len);
      // trying to append bytes to a sub-message of a longer chain, have to fork the chain
      fork_message_chain(raw);
      mp = raw->last;
    }
    assert(mp && !mp->next && raw->last_offset == mp->offset + mp->len);
    msg_buffer_t *buffer = mp->buffer;
    if (msg_buffer_use_count(buffer) == 1) {
      const int buffer_size = msg_buffer_size(buffer);
      const int sz = buffer_size - raw->last_offset;
      assert(sz >= 0 && sz <= buffer_size);
      const int to_copy = std::min(sz, alloc_bytes);
      if (to_copy) {
        if (data) {
          memcpy(buffer->data + raw->last_offset, data, to_copy);
          data = static_cast<const char *>(data) + to_copy;
        }
        raw->total_bytes += to_copy;
        raw->last_offset += to_copy;
        mp->len += to_copy;
        alloc_bytes -= to_copy;
      }
      res = to_copy;
    }
  }
  while (alloc_bytes > 0) {
    mpl = mp;
    msg_buffer_t *buffer = alloc_msg_buffer(raw->total_bytes + alloc_bytes >= std_buffer ? std_buffer : small_buffer);
    if (!buffer) {
      return res;
    }
    mp = new_msg_part(buffer);
    mpl->next = raw->last = mp;
    const int buffer_size = msg_buffer_size(buffer);
    const int to_copy = std::min(buffer_size, alloc_bytes);
    mp->len = to_copy;
    raw->total_bytes += to_copy;
    raw->last_offset = to_copy;
    if (data) {
      memcpy(buffer->data, data, to_copy);
      data = static_cast<const char *>(data) + to_copy;
    }
    alloc_bytes -= to_copy;
    res += to_copy;
  }

  return res;
}

int rwm_push_data(raw_message_t *raw, const void *data, int alloc_bytes) {
  return rwm_push_data_ext(raw, data, alloc_bytes, RM_PREPEND_RESERVE, MSG_SMALL_BUFFER, MSG_STD_BUFFER);
}

int rwm_push_data_front(raw_message_t *raw, const void *data, int alloc_bytes) {
  assert(raw->magic == RM_INIT_MAGIC);
  assert(alloc_bytes >= 0);
  if (!alloc_bytes) {
    return 0;
  }
  msg_part_t *mp = NULL;
  const int r = alloc_bytes;
  if (raw->first) {
    msg_buffer_t *buffer = raw->first->buffer;
    if (msg_part_use_count(raw->first) > 1) {
      if (raw->first_offset != raw->first->offset) {
        mp = new_msg_part(buffer);
        msg_buffer_inc_ref(buffer);
        mp->offset = raw->first_offset;
        mp->len = raw->first->len + (raw->first->offset - raw->first_offset);
        mp->next = raw->first->next;
        if (mp->next) {
          msg_part_inc_ref(mp->next);
        }
        if (raw->last == raw->first) {
          raw->last = mp;
          int delta = mp->offset + mp->len - raw->last_offset;
          assert(delta >= 0);
          mp->len -= delta;
        }
        msg_part_chain_dec_ref(raw->first);
        raw->first = mp;
      } else {
        mp = raw->first;
      }
    } else {
      fixup_msg_part_to_rwm(raw, raw->first);
      mp = raw->first;
    }
    if (msg_part_use_count(mp) == 1 && msg_buffer_use_count(buffer) == 1) {
      const int to_copy = std::min(alloc_bytes, raw->first_offset);
      memcpy(buffer->data + raw->first_offset - to_copy, static_cast<const char *>(data) + (alloc_bytes - to_copy), to_copy);
      raw->first->len += to_copy;
      raw->first->offset -= to_copy;
      raw->first_offset = raw->first->offset;
      raw->total_bytes += to_copy;
      alloc_bytes -= to_copy;
    }
  }

  while (alloc_bytes) {
    msg_buffer_t *buffer = alloc_msg_buffer(alloc_bytes >= MSG_SMALL_BUFFER ? MSG_STD_BUFFER : MSG_SMALL_BUFFER);
    assert(buffer);
    const int buffer_size = msg_buffer_size(buffer);
    mp = new_msg_part(buffer);
    mp->next = raw->first;
    raw->first = mp;

    const int to_copy = std::min(alloc_bytes, buffer_size);

    memcpy(buffer->data + buffer_size - to_copy, static_cast<const char *>(data) + (alloc_bytes - to_copy), to_copy);
    mp->len = to_copy;
    mp->offset = (buffer_size - to_copy);
    raw->first_offset = mp->offset;
    raw->total_bytes += to_copy;
    if (!raw->last) {
      raw->last = mp;
      raw->last_offset = mp->offset + mp->len;
    }
    alloc_bytes -= to_copy;
  }

  return r;
}

int rwm_create(raw_message_t *raw, const void *data, int alloc_bytes) {
  PARALLEL_COUNTER_INC(rwm_total_msgs);
  memset(raw, 0, sizeof(*raw));
  raw->magic = RM_INIT_MAGIC;
  return rwm_push_data(raw, data, alloc_bytes);
}

void rwm_init(raw_message_t *raw, int alloc_bytes) {
  rwm_create(raw, NULL, alloc_bytes);
}

void *rwm_prepend_alloc(raw_message_t *raw, int alloc_bytes) {
  assert(raw->magic == RM_INIT_MAGIC);
  assert(alloc_bytes >= 0);
  if (!alloc_bytes || alloc_bytes > MSG_STD_BUFFER) {
    return NULL;
  }
  // msg_part_t *mp, *mpl;
  // int res = 0;
  if (!raw->first) {
    rwm_push_data(raw, NULL, alloc_bytes);
    return raw->first->buffer->data + raw->first_offset;
  }
  if (msg_part_use_count(raw->first) == 1) {
    if (raw->first_offset != raw->first->offset) {
      fixup_msg_part_to_rwm(raw, raw->first);
    }
    if (raw->first->offset >= alloc_bytes && msg_buffer_use_count(raw->first->buffer) == 1) {
      raw->first->offset -= alloc_bytes;
      raw->first->len += alloc_bytes;
      raw->first_offset -= alloc_bytes;
      raw->total_bytes += alloc_bytes;
      return raw->first->buffer->data + raw->first_offset;
    }
  } else {
    if (raw->first_offset != raw->first->offset) {
      msg_part_t *mp = new_msg_part(raw->first->buffer);
      msg_buffer_inc_ref(raw->first->buffer);
      mp->next = raw->first->next;
      if (mp->next) {
        msg_part_inc_ref(mp->next);
      }

      mp->offset = raw->first_offset;
      mp->len = raw->first->len - (raw->first_offset - raw->first->offset);

      if (raw->last == raw->first) {
        raw->last = mp;
      }
      msg_part_chain_dec_ref(raw->first);
      raw->first = mp;
    }
  }
  assert(raw->first_offset == raw->first->offset);
  msg_buffer_t *X = alloc_msg_buffer(alloc_bytes);
  assert(X);
  const int size = msg_buffer_size(X);
  assert(size >= alloc_bytes);
  msg_part_t *mp = new_msg_part(X);
  mp->next = raw->first;
  raw->first = mp;
  mp->len = alloc_bytes;
  mp->offset = size - alloc_bytes;
  raw->first_offset = mp->offset;
  raw->total_bytes += alloc_bytes;
  return raw->first->buffer->data + mp->offset;
}

void *rwm_postpone_alloc_ext(raw_message_t *raw, int min_alloc_bytes, int max_alloc_bytes, int prepend_reserve, int continue_buffer, int small_buffer,
                             int std_buffer, int *allocated) {
  assert(raw->magic == RM_INIT_MAGIC);
  assert(max_alloc_bytes >= 0);
  if (!max_alloc_bytes || max_alloc_bytes > std_buffer) {
    return nullptr;
  }
  if (!raw->first) {
    rwm_push_data_ext(raw, nullptr, max_alloc_bytes, prepend_reserve, small_buffer, std_buffer);
    msg_part_t *mp = raw->first;
    *allocated = (mp == raw->last ? raw->last_offset : mp->offset + mp->len) - raw->first_offset;
    return raw->first->buffer->data + raw->first_offset;
  }

  if (raw->last->next || raw->last->len + raw->last->offset != raw->last_offset) {
    fork_message_chain(raw);
  }
  msg_part_t *mp = raw->last;
  int size = msg_buffer_size(mp->buffer);
  if (size - mp->len - mp->offset >= min_alloc_bytes && msg_buffer_use_count(mp->buffer) == 1) {
    *allocated = std::min(max_alloc_bytes, size - mp->len - mp->offset);
    raw->total_bytes += *allocated;
    mp->len += *allocated;
    raw->last_offset += *allocated;
    return mp->buffer->data + mp->offset + mp->len - *allocated;
  }
  *allocated = max_alloc_bytes;
  msg_buffer_t *X = alloc_msg_buffer(continue_buffer);
  assert(X);
  size = msg_buffer_size(X);
  assert(size >= max_alloc_bytes);

  mp = new_msg_part(X);
  raw->last->next = mp;
  raw->last = mp;

  mp->len = max_alloc_bytes;
  mp->offset = 0;
  raw->last_offset = max_alloc_bytes;
  raw->total_bytes += max_alloc_bytes;

  return mp->buffer->data;
}

void *rwm_postpone_alloc(raw_message_t *raw, int alloc_bytes) {
  int unused = 0;
  return rwm_postpone_alloc_ext(raw, alloc_bytes, alloc_bytes, RM_PREPEND_RESERVE, alloc_bytes, MSG_SMALL_BUFFER, MSG_STD_BUFFER, &unused);
}

int rwm_prepare_iovec(const raw_message_t *raw, struct iovec *iov, int iov_len, int bytes) {
  assert(raw->magic == RM_INIT_MAGIC);
  if (bytes > raw->total_bytes) {
    bytes = raw->total_bytes;
  }
  assert(bytes >= 0);
  int res = 0, total_bytes = raw->total_bytes, first_offset = raw->first_offset;
  msg_part_t *mp = raw->first;
  while (bytes > 0) {
    assert(mp);
    if (res == iov_len) {
      return -1;
    }
    const int sz = msg_part_last_offset(raw, mp) - first_offset;
    assert(sz >= 0 && sz <= mp->len);
    if (bytes < sz) {
      iov[res].iov_base = mp->buffer->data + first_offset;
      iov[res++].iov_len = bytes;
      return res;
    }
    iov[res].iov_base = mp->buffer->data + first_offset;
    iov[res++].iov_len = sz;
    bytes -= sz;
    total_bytes -= sz;
    if (!mp->next) {
      assert(mp == raw->last && !bytes && !total_bytes);
      return res;
    }
    mp = mp->next;
    first_offset = mp->offset;
  }
  return res;
}

int rwm_fetch_data_back(raw_message_t *raw, void *data, int bytes) {
  assert(raw->magic == RM_INIT_MAGIC);
  if (bytes > raw->total_bytes) {
    bytes = raw->total_bytes;
  }
  assert(bytes >= 0);
  if (!bytes) {
    return 0;
  }

  if (bytes < raw->last_offset - msg_part_first_offset(raw, raw->last)) {
    if (data) {
      memcpy(data, raw->last->buffer->data + raw->last_offset - bytes, bytes);
    }
    raw->last_offset -= bytes;
    raw->total_bytes -= bytes;
    if (!raw->total_bytes) {
      rwm_clear(raw);
    }
    return bytes;
  }

  int skip = raw->total_bytes - bytes;
  int res = 0;
  msg_part_t *mp = raw->first;
  while (bytes > 0) {
    assert(mp);
    const int sz = msg_part_last_offset(raw, mp) - msg_part_first_offset(raw, mp);
    assert(sz >= 0 && sz <= mp->len);
    if (skip <= sz) {
      raw->last = mp;
      raw->last_offset = msg_part_first_offset(raw, mp) + skip;

      //      int t = mp->len + mp->offset - raw->last_offset;
      int t = sz - skip;
      if (t > bytes) {
        t = bytes;
      }
      if (data) {
        memcpy(data, mp->buffer->data + raw->last_offset, t);
      }
      bytes -= (t);
      res += (t);

      if (data) {
        msg_part_t *m = mp->next;
        while (bytes) {
          assert(m);
          int t = (m->len > bytes) ? bytes : m->len;
          memcpy(static_cast<char *>(data) + res, m->buffer->data + m->offset, t);
          bytes -= t;
          res += t;
          m = m->next;
        }
      } else {
        res += bytes;
        bytes = 0;
      }
    } else {
      assert(mp != raw->last);
      skip -= sz;
      mp = mp->next;
    }
  }
  raw->total_bytes -= res;
  if (!raw->total_bytes) {
    msg_part_chain_dec_ref(raw->first);
    raw->first = raw->last = NULL;
    raw->first_offset = raw->last_offset = 0;
  }
  return res;
}

int rwm_trunc(raw_message_t *raw, int len) {
  if (len >= raw->total_bytes) {
    return raw->total_bytes;
  }
  rwm_fetch_data_back(raw, NULL, raw->total_bytes - len);
  return len;
}

int rwm_split(raw_message_t *raw, raw_message_t *tail, int bytes) {
  assert(bytes >= 0);
  PARALLEL_COUNTER_INC(rwm_total_msgs);
  tail->magic = raw->magic;
  if (bytes >= raw->total_bytes) {
    tail->first = tail->last = NULL;
    tail->first_offset = tail->last_offset = 0;
    tail->total_bytes = 0;
    return bytes == raw->total_bytes ? 0 : -1;
  }
  if (raw->total_bytes - bytes <= raw->last_offset - raw->last->offset) {
    int s = raw->total_bytes - bytes;
    raw->last_offset -= s;
    raw->total_bytes -= s;
    tail->first = tail->last = raw->last;
    msg_part_inc_ref(tail->first);
    tail->first_offset = raw->last_offset;
    tail->last_offset = raw->last_offset + s;
    tail->total_bytes = s;
    return 0;
  }
  tail->total_bytes = raw->total_bytes - bytes;
  raw->total_bytes = bytes;
  msg_part_t *mp = raw->first;
  while (bytes) {
    assert(mp);
    const int sz = msg_part_last_offset(raw, mp) - msg_part_first_offset(raw, mp);
    if (sz < bytes) {
      bytes -= sz;
      mp = mp->next;
    } else {
      tail->last = raw->last;
      tail->last_offset = raw->last_offset;
      raw->last = mp;
      raw->last_offset = msg_part_first_offset(raw, mp) + bytes;
      tail->first = mp;
      tail->first_offset = raw->last_offset;
      msg_part_inc_ref(mp);
      bytes = 0;
    }
  }
  return 0;
}

int rwm_split_head(raw_message_t *head, raw_message_t *raw, int bytes) {
  *head = *raw;
  return rwm_split(head, raw, bytes);
}

int rwm_union(raw_message_t *raw, raw_message_t *tail) {
  //  assert (raw != tail);
  if (!raw->last) {
    *raw = *tail;
    PARALLEL_COUNTER_DEC(rwm_total_msgs);
    tail->magic = 0;
    return 0;
  } else if (tail->first) {

    if (raw->last->next || raw->last->len + raw->last->offset != raw->last_offset) {
      fork_message_chain(raw);
    } else {
      msg_part_t *mp = tail->last;
      while (mp->next) {
        mp = mp->next;
      }
      if (mp == raw->last) {
        fork_message_chain(raw);
      }
    }

    if (tail->first && tail->first->buffer == raw->last->buffer && tail->first_offset == raw->last_offset) {
      raw->last->next = tail->first->next;
      if (tail->first->next) {
        msg_part_inc_ref(tail->first->next);
      }
      int end = (tail->first == tail->last) ? tail->last_offset : tail->first->offset + tail->first->len;
      raw->last->len += end - tail->first_offset;

      raw->last_offset = tail->last_offset;
      if (tail->first != tail->last) {
        raw->last = tail->last;
      }
      raw->total_bytes += tail->total_bytes;
    } else {

      msg_part_t *mp = NULL;
      if (msg_part_use_count(tail->first) != 1 && tail->first_offset != tail->first->offset) {
        mp = new_msg_part(tail->first->buffer);
        mp->next = tail->first->next;
        if (mp->next) {
          msg_part_inc_ref(mp->next);
        }
        mp->offset = tail->first_offset;
        mp->len = tail->first->len - tail->first_offset + tail->first->offset;
        msg_buffer_inc_ref(mp->buffer);
        raw->last->next = mp;
      } else {
        raw->last->next = tail->first;
        msg_part_inc_ref(tail->first);
        tail->first->len = tail->first->offset + tail->first->len - tail->first_offset;
        tail->first->offset = tail->first_offset;
      }

      raw->total_bytes += tail->total_bytes;
      if (tail->first != tail->last || !mp) {
        raw->last = tail->last;
        raw->last_offset = tail->last_offset;
      } else {
        raw->last = mp;
        raw->last_offset = tail->last_offset;
      }
    }
  }
  rwm_free(tail);
  return 0;
}

void rwm_union_unique(raw_message_t *head, raw_message_t *tail) {
  assert(head != tail);
  assert(head->first && head->last);
  assert(msg_part_use_count(head->first) == 1 && msg_part_use_count(head->last) == 1);
  assert(tail->first && tail->last);
  assert(msg_part_use_count(tail->first) == 1 && msg_part_use_count(tail->last) == 1);
  assert(!head->last->next && !tail->last->next);
  assert(head->last_offset == head->last->offset + head->last->len);
  assert(head->first_offset == head->first->offset);
  assert(tail->first_offset == tail->first->offset);
  assert(tail->last_offset == tail->last->offset + tail->last->len);

  head->total_bytes += tail->total_bytes;
  head->last->next = tail->first;
  head->last = tail->last;
  head->last_offset = tail->last_offset;
  PARALLEL_COUNTER_DEC(rwm_total_msgs);
  tail->magic = 0;
}

void rwm_dump_sizes(raw_message_t *raw) {
  if (!raw->first) {
    fprintf(stderr, "( ) # %d\n", raw->total_bytes);
    assert(!raw->total_bytes);
  } else {
    int total_size = 0;
    msg_part_t *mp = raw->first;
    fprintf(stderr, "(");
    while (mp != NULL) {
      const int size = msg_part_last_offset(raw, mp) - msg_part_first_offset(raw, mp);
      fprintf(stderr, " %d", size);
      total_size += size;
      if (mp == raw->last) {
        break;
      }
      mp = mp->next;
    }
    assert(mp == raw->last);
    fprintf(stderr, " ) # %d\n", raw->total_bytes);
    assert(total_size == raw->total_bytes);
  }
}

void rwm_check(raw_message_t *raw) {
  assert(raw->magic == RM_INIT_MAGIC);
  if (!raw->first) {
    assert(!raw->total_bytes);
  } else {
    int total_size = 0;
    msg_part_t *mp = raw->first;
    while (mp != NULL) {
      const int size = msg_part_last_offset(raw, mp) - msg_part_first_offset(raw, mp);
      total_size += size;
      if (mp == raw->last) {
        break;
      }
      mp = mp->next;
    }
    assert(mp == raw->last);
    assert(total_size == raw->total_bytes);
  }
}

void rwm_dump(raw_message_t *raw) {
  assert(raw->magic == RM_INIT_MAGIC);
  raw_message_t t;
  rwm_clone(&t, raw);
  char R[10004];
  int r = rwm_fetch_data(&t, R, 10004);
  int x = (r > 10000) ? 10000 : r;
  hexdump(stderr, R, R + x);
  if (r > x) {
    fprintf(stderr, "%d bytes not printed\n", raw->total_bytes - x);
  }
  rwm_free(&t);
}

template<class Func>
static inline int rwm_process_and_advance_internal(raw_message_t *raw, int bytes, const Func &callback) {
  assert(raw->magic == RM_INIT_MAGIC);
  if (bytes > raw->total_bytes) {
    bytes = raw->total_bytes;
  }
  assert(bytes >= 0);
  if (!bytes) {
    return 0;
  }
  int res = 0;
  msg_part_t *mp = raw->first, *mpn;
  while (bytes > 0) {
    assert(mp);
    const int sz = msg_part_last_offset(raw, mp) - raw->first_offset;
    assert(sz >= 0 && sz <= mp->len);
    if (bytes < sz) {
      if (bytes > 0) {
        callback(mp->buffer->data + raw->first_offset, bytes);
      }
      raw->first_offset += bytes;
      raw->total_bytes -= bytes;
      return res + bytes;
    }
    if (sz > 0) {
      callback(mp->buffer->data + raw->first_offset, sz);
    }
    res += sz;
    bytes -= sz;
    raw->total_bytes -= sz;
    mpn = mp->next;
    int deleted = 0;
    if (raw->magic == RM_INIT_MAGIC && !msg_part_dec_ref(mp)) {
      deleted = 1;
    }
    if (!mpn || raw->first == raw->last) {
      if (mpn && deleted) {
        msg_part_chain_dec_ref(mpn);
      }
      assert(mp == raw->last && !bytes && !raw->total_bytes);
      raw->first = raw->last = NULL;
      raw->first_offset = raw->last_offset = 0;
      return res;
    }
    mp = mpn;
    raw->first = mp;
    raw->first_offset = mp->offset;
    assert(msg_part_use_count(mp) > 0);
    if (!deleted) {
      msg_part_inc_ref(mp);
    }
  }
  return res;
}

int rwm_process_and_advance(raw_message_t *raw, int bytes, const std::function<void(const void *, int)> &callback) {
  return rwm_process_and_advance_internal(raw, bytes, callback);
}

int rwm_process_and_advance(raw_message_t *raw, int bytes, rwm_process_callback_t cb, void *extra) {
  return rwm_process_and_advance_internal(raw, bytes, [cb, extra](const void *buf, int len) { return cb(extra, buf, len); });
}

template<class Func>
int rwm_process_from_offset_internal(const raw_message_t *raw, int bytes, int offset, const Func &callback) {
  assert(bytes >= 0);
  assert(offset >= 0);
  if (bytes + offset > raw->total_bytes) {
    bytes = raw->total_bytes - offset;
  }
  if (bytes <= 0) {
    return 0;
  }
  const int x = bytes;
  msg_part_t *mp = raw->first;
  while (mp) {
    int start = msg_part_first_offset(raw, mp);
    int len = msg_part_last_offset(raw, mp) - start;
    if (len > offset) {
      start += offset;
      len -= offset;
      for (;;) {
        if (len >= bytes) {
          int r = callback(mp->buffer->data + start, bytes);
          return (r < 0) ? r : x;
        }
        int r = callback(mp->buffer->data + start, len);
        if (r < 0) {
          return r;
        }
        bytes -= len;
        mp = mp->next;
        if (!mp) {
          return x;
        }
        start = msg_part_first_offset(raw, mp);
        len = msg_part_last_offset(raw, mp) - start;
      }
    }
    offset -= len;
    mp = mp->next;
  }
  return x;
}

int rwm_process_from_offset(const raw_message_t *raw, int bytes, int offset, rwm_process_callback_t cb, void *extra) {
  return rwm_process_from_offset_internal(raw, bytes, offset, [&](const void *data, int len) { return cb(extra, data, len); });
}

int rwm_process(const raw_message_t *raw, int bytes, rwm_process_callback_t cb, void *extra) {
  return rwm_process_from_offset(raw, bytes, 0, cb, extra);
}

int rwm_process(const raw_message_t *raw, int bytes, const std::function<int(const void *, int)> &callback) {
  return rwm_process_from_offset_internal(raw, bytes, 0, callback);
}

int rwm_transform_from_offset(raw_message_t *raw, int bytes, int offset, rwm_transform_callback_t cb, void *extra) {
  assert(bytes >= 0 && offset >= 0);
  if (bytes + offset > raw->total_bytes) {
    bytes = raw->total_bytes - offset;
  }
  if (bytes <= 0) {
    return 0;
  }
  int x = bytes, r;
  msg_part_t *mp = raw->first;
  while (mp) {
    int start = msg_part_first_offset(raw, mp);
    int len = msg_part_last_offset(raw, mp) - start;
    if (len > offset) {
      start += offset;
      len -= offset;
      for (;;) {
        assert(msg_part_use_count(mp) == 1);
        if (len >= bytes) {
          int r = cb(extra, mp->buffer->data + start, bytes);
          return (r < 0) ? r : x;
        }
        r = cb(extra, mp->buffer->data + start, len);
        if (r < 0) {
          return r;
        }
        bytes -= len;
        mp = mp->next;
        if (!mp) {
          return x;
        }
        start = msg_part_first_offset(raw, mp);
        len = msg_part_last_offset(raw, mp) - start;
      }
    }
    offset -= len;
    mp = mp->next;
  }
  return x;
}

static int crc32c_process(void *extra, const void *data, int len) {
  uint32_t crc32c = *(uint32_t *)extra;
  *(uint32_t *)extra = crc32c_partial(data, len, crc32c);
  return 0;
}

uint32_t rwm_crc32c(const raw_message_t *raw, int bytes) {
  uint32_t crc32c = ~0;

  rwm_process_callback_t cb = crc32c_process;
  assert(rwm_process(raw, bytes, cb, &crc32c) == bytes);

  return ~crc32c;
}

static int crc32_process(void *extra, const void *data, int len) {
  uint32_t crc32 = *(uint32_t *)extra;
  *(uint32_t *)extra = crc32_partial(data, len, crc32);
  return 0;
}

uint32_t rwm_crc32(const raw_message_t *raw, int bytes) {
  uint32_t crc32 = ~0;

  rwm_process_callback_t cb = crc32_process;
  assert(rwm_process(raw, bytes, cb, &crc32) == bytes);

  return ~crc32;
}

struct custom_crc32_data {
  crc32_partial_func_t partial;
  uint32_t crc32;
};

static int custom_crc32_process(void *extra, const void *data, int len) {
  struct custom_crc32_data *DP = static_cast<custom_crc32_data *>(extra);
  DP->crc32 = DP->partial(data, len, DP->crc32);
  return 0;
}

uint32_t rwm_custom_crc32(const raw_message_t *raw, int bytes, crc32_partial_func_t custom_crc32_partial) {
  struct custom_crc32_data D;
  D.partial = custom_crc32_partial;
  D.crc32 = -1;

  rwm_process_callback_t cb = custom_crc32_process;
  assert(rwm_process(raw, bytes, cb, &D) == bytes);

  return ~D.crc32;
}

static int rwm_process_memcpy(void *extra, const void *data, int len) {
  char **d = static_cast<char **>(extra);
  memcpy(*d, data, len);
  *d += len;
  return 0;
}

static int rwm_process_nop(void *extra __attribute__((unused)), const void *data __attribute__((unused)), int len __attribute__((unused))) {
  return 0;
}

int rwm_fetch_data(raw_message_t *raw, void *buf, int bytes) {
  if (buf) {
    return rwm_process_and_advance(raw, bytes, rwm_process_memcpy, &buf);
  } else {
    return rwm_process_and_advance(raw, bytes, rwm_process_nop, NULL);
  }
}

int rwm_fetch_lookup(const raw_message_t *raw, void *buf, int bytes) {
  if (buf) {
    return rwm_process(raw, bytes, rwm_process_memcpy, &buf);
  } else {
    return rwm_process(raw, bytes, rwm_process_nop, NULL);
  }
}

static int rwm_process_fork(void *extra, const void *data, int len) {
  raw_message_t *forked_rwm = static_cast<raw_message_t *>(extra);
  assert(rwm_push_data(forked_rwm, data, len) == len);

  return 0;
}

void rwm_fork_deep(raw_message_t *raw) {
  for (msg_part_t *mp = raw->first; mp; mp = mp->next) {
    if (msg_part_use_count(mp) != 1 || msg_buffer_use_count(mp->buffer) != 1) {
      raw_message_t rwm_forked;
      rwm_init(&rwm_forked, 0);
      rwm_process(raw, raw->total_bytes, rwm_process_fork, &rwm_forked);
      rwm_free(raw);
      *raw = rwm_forked;
      return;
    }
  }
}

struct rwm_encrypt_decrypt_tmp {
  char buf[16];
  int bp;
  int buf_left;
  int left;
  raw_message_t *raw;
  struct vk_aes_ctx *ctx;
  rwm_encrypt_decrypt_to_callback_t callback;
  unsigned char *iv;
};

static int rwm_process_encrypt_decrypt(void *extra, const void *data, int len) {
  rwm_encrypt_decrypt_tmp *x = static_cast<rwm_encrypt_decrypt_tmp *>(extra);
  raw_message_t *res = x->raw;
  if (!x->buf_left) {
    msg_buffer_t *X = alloc_msg_buffer(x->left >= MSG_STD_BUFFER ? MSG_STD_BUFFER : x->left);
    assert(X);
    msg_part_t *mp = new_msg_part(X);
    res->last->next = mp;
    res->last = mp;
    res->last_offset = 0;
    x->buf_left = msg_buffer_size(X);
  }
  x->left -= len;
  if (x->bp) {
    if (len >= 16 - x->bp) {
      memcpy(x->buf + x->bp, data, 16 - x->bp);
      len -= 16 - x->bp;
      data = static_cast<const char *>(data) + 16 - x->bp;
      x->bp = 0;
      x->callback(x->ctx, x->buf, res->last->buffer->data + res->last_offset, 16, x->iv);
      res->last->len += 16;
      res->last_offset += 16;
      res->total_bytes += 16;
      x->buf_left -= 16;
    } else {
      memcpy(x->buf + x->bp, data, len);
      x->bp += len;
      return 0;
    }
  }
  if (len & 15) {
    int l = len & ~15;
    memcpy(x->buf, static_cast<const char *>(data) + l, len - l);
    x->bp = len - l;
    len = l;
  }
  while (true) {
    if (!x->buf_left) {
      msg_buffer_t *X = alloc_msg_buffer(x->left + len >= MSG_STD_BUFFER ? MSG_STD_BUFFER : x->left + len);
      assert(X);
      msg_part_t *mp = new_msg_part(X);
      res->last->next = mp;
      res->last = mp;
      res->last_offset = 0;
      x->buf_left = msg_buffer_size(X);
    }
    if (len <= x->buf_left) {
      x->callback(x->ctx, data, (res->last->buffer->data + res->last_offset), len, x->iv);
      res->last->len += len;
      res->last_offset += len;
      res->total_bytes += len;
      x->buf_left -= len;
      return 0;
    } else {
      int t = x->buf_left;
      x->callback(x->ctx, data, res->last->buffer->data + res->last_offset, t, x->iv);
      res->last->len += t;
      res->last_offset += t;
      res->total_bytes += t;
      data = static_cast<const char *>(data) + t;
      len -= t;
      x->buf_left -= t;
    }
  }

  return 0;
}

int rwm_encrypt_decrypt_to(raw_message_t *raw, raw_message_t *res, int bytes, struct vk_aes_ctx *ctx, rwm_encrypt_decrypt_to_callback_t crypt_cb,
                           unsigned char *iv) {
  assert(bytes >= 0);
  if (bytes > raw->total_bytes) {
    bytes = raw->total_bytes;
  }
  bytes &= ~15;
  if (!bytes) {
    return 0;
  }
  if (res->last && (res->last->next || res->last->offset != res->last_offset)) {
    fork_message_chain(res);
  }
  if (!res->last || msg_buffer_use_count(res->last->buffer) != 1) {
    int l = res->last ? bytes : bytes + RM_PREPEND_RESERVE;
    msg_buffer_t *X = alloc_msg_buffer(l >= MSG_STD_BUFFER ? MSG_STD_BUFFER : l);
    assert(X);
    msg_part_t *mp = new_msg_part(X);
    if (res->last) {
      res->last->next = mp;
      res->last = mp;
      res->last_offset = 0;
    } else {
      res->last = res->first = mp;
      res->last_offset = res->first_offset = mp->offset = RM_PREPEND_RESERVE;
      mp->len = 0;
    }
  }
  struct rwm_encrypt_decrypt_tmp t;
  memset(&t, 0, sizeof(t));
  assert(!t.bp);
  t.callback = crypt_cb;
  t.buf_left = msg_buffer_size(res->last->buffer) - res->last_offset;
  t.raw = res;
  t.ctx = ctx;
  t.iv = iv;
  t.left = bytes;

  rwm_process_callback_t cb = rwm_process_encrypt_decrypt;
  return rwm_process_and_advance(raw, bytes, cb, &t);
}

static int _rwm_encrypt_decrypt(raw_message_t *raw, int bytes, struct vk_aes_ctx *ctx, int mode, unsigned char *iv) {
  assert(bytes >= 0);
  if (bytes > raw->total_bytes) {
    bytes = raw->total_bytes;
  }
  bytes &= ~15;
  if (!bytes) {
    return 0;
  }
  int s = bytes;
  rwm_fork_deep(raw);
  //  assert (raw->total_bytes % 16 == 0);

  msg_part_t *mp = raw->first;
  int start = msg_part_first_offset(raw, mp);
  int len = msg_part_last_offset(raw, mp) - start;
  while (bytes) {
    assert(start >= 0);
    assert(len >= 0);
    while (len >= 16) {
      int l = len < bytes ? (len & ~15) : bytes;
      if (!mode) {
        ctx->ige_crypt(ctx, reinterpret_cast<const uint8_t *>(mp->buffer->data + start), reinterpret_cast<uint8_t *>(mp->buffer->data + start), l, iv);
      } else {
        ctx->cbc_crypt(ctx, reinterpret_cast<const uint8_t *>(mp->buffer->data + start), reinterpret_cast<uint8_t *>(mp->buffer->data + start), l, iv);
      }
      start += l;
      bytes -= l;
      len = len & 15;
    }
    if (len && bytes) {
      unsigned char c[16];
      int p = 0;
      int _len = len;
      int _start = start;
      msg_part_t *_mp = mp;
      while (p < 16) {
        int x = (len > 16 - p) ? 16 - p : len;
        memcpy(c + p, mp->buffer->data + start, x);
        p += x;
        if (len == x) {
          mp = mp->next;
          if (mp) {
            start = mp->offset;
            len = (mp == raw->last) ? raw->last_offset - start : mp->len;
          } else {
            start = -1;
            len = -1;
            assert(p == 16);
          }
        } else {
          break;
        }
      }
      if (!mode) {
        ctx->ige_crypt(ctx, c, c, 16, iv);
      } else {
        ctx->cbc_crypt(ctx, c, c, 16, iv);
      }
      len = _len;
      start = _start;
      mp = _mp;
      p = 0;
      while (p < 16) {
        int x = (len > 16 - p) ? 16 - p : len;
        memcpy(mp->buffer->data + start, c + p, x);
        p += x;
        if (len == x) {
          mp = mp->next;
          if (mp) {
            start = mp->offset;
            len = (mp == raw->last) ? raw->last_offset - start : mp->len;
          } else {
            start = -1;
            len = -1;
            assert(p == 16);
          }
        } else {
          start += x;
          len -= x;
          break;
        }
      }
      bytes -= 16;
    } else {
      mp = mp->next;
      if (mp) {
        start = mp->offset;
        len = (mp == raw->last) ? raw->last_offset - start : mp->len;
      } else {
        start = -1;
        len = -1;
      }
    }
  }
  return s;
}

int rwm_encrypt_decrypt(raw_message_t *raw, int bytes, struct vk_aes_ctx *ctx, unsigned char iv[32]) {
  return _rwm_encrypt_decrypt(raw, bytes, ctx, 0, iv);
}

int rwm_encrypt_decrypt_cbc(raw_message_t *raw, int bytes, struct vk_aes_ctx *ctx, unsigned char iv[16]) {
  return _rwm_encrypt_decrypt(raw, bytes, ctx, 1, iv);
}
