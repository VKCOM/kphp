// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __NET_BUFFERS_H__
#define __NET_BUFFERS_H__

#include <sys/cdefs.h>

#include "common/crc32.h"

typedef struct netbuffer netbuffer_t;

struct netbuffer {
  int state;
  void *parent;
  netbuffer_t *prev, *next;
  char *rptr, *pptr, *wptr;
  char *start, *end;
  int extra;
  int total_bytes;       /* if pptr present, processed bytes only - rptr..pptr */
  int unprocessed_bytes; /* if pptr present, pptr..wptr, else 0 */
};

typedef struct nb_iterator {
  netbuffer_t *head;
  netbuffer_t *cur;
  char *cptr;
} nb_iterator_t;

typedef struct nb_iterator nbw_iterator_t;

typedef struct nb_process_iterator {
  netbuffer_t *head;
  netbuffer_t *cur;
  char *ptr0, *ptr1;
  int len0, len1;
} nb_processor_t;

#define NB_MAGIC_FREE 0x16432332
#define NB_MAGIC_BUSY 0x5a2219b4
#define NB_MAGIC_BUSY_MAX (NB_MAGIC_BUSY + 0x1000)
#define NB_MAGIC_HEAD 0x6ab246ae
#define NB_MAGIC_BUSYHEAD 0x47a5e431
#define NB_MAGIC_SUB 0x276f35d3
#define NB_MAGIC_ALLOCA 0x3bd902a5

extern int NB_used, NB_alloc, NB_free, NB_max, NB_size;

void init_netbuffers();

netbuffer_t *alloc_buffer();
netbuffer_t *alloc_head_buffer();
int free_buffer(netbuffer_t *R);

netbuffer_t *init_builtin_buffer(netbuffer_t *H, char *buf, int len);

void free_unused_buffers(netbuffer_t *H);
void free_all_buffers(netbuffer_t *H);

char *get_read_ptr(netbuffer_t *H);
char *get_write_ptr(netbuffer_t *H, int len);
int get_ready_bytes(netbuffer_t *H);
int get_write_space(netbuffer_t *H);
int get_total_ready_bytes(netbuffer_t *H);

int force_ready_bytes(netbuffer_t *H, int sz);

void advance_read_ptr(netbuffer_t *H, int offset);
void advance_write_ptr(netbuffer_t *H, int offset);
int advance_skip_read_ptr(netbuffer_t *H, int len);

int write_out(netbuffer_t *H, const void *data, int len);
int read_in(netbuffer_t *H, void *data, int len);
int copy_through(netbuffer_t *HD, netbuffer_t *HS, int len);
int copy_through_nondestruct(netbuffer_t *HD, netbuffer_t *HS, int len);
// reads_back unprocessed data
int read_back(netbuffer_t *H, void *data, int len);
int read_back_nondestruct(netbuffer_t *H, void *__data, int len);
unsigned count_crc32_back_partial(netbuffer_t *H, int len, unsigned complement_crc32);
unsigned count_crc32_back_partial_ex(netbuffer_t *H, int len, unsigned complement_crc32, crc32_partial_func_t crc_f);

typedef netbuffer_t **nb_allocator_t;

void *nb_alloc(netbuffer_t *H, int len);
void *nbr_alloc(nb_allocator_t pH, int len);

int nb_start_process(nb_processor_t *P, netbuffer_t *H);
int nb_advance_process(nb_processor_t *P, int offset);

int nbit_copy_through_nondestruct(netbuffer_t *XD, nb_iterator_t *I, int len);
int nbit_copy_through(netbuffer_t *XD, nb_iterator_t *I, int len);

int mark_all_processed(netbuffer_t *H);
int mark_all_unprocessed(netbuffer_t *H);
int release_all_unprocessed(netbuffer_t *H);

int nbit_set(nb_iterator_t *I, netbuffer_t *H);
int nbit_clear(nb_iterator_t *I);
int nbit_rewind(nb_iterator_t *I);
int nbit_total_ready_bytes(nb_iterator_t *I);
int nbit_ready_bytes(nb_iterator_t *I);
void *nbit_get_ptr(nb_iterator_t *I);
int nbit_advance(nb_iterator_t *I, int offset);
int nbit_read_in(nb_iterator_t *I, void *data, int len);

int nbit_setw(nbw_iterator_t *IW, netbuffer_t *H);
int nbit_clearw(nbw_iterator_t *IW);
int nbit_rewindw(nbw_iterator_t *IW);
int nbit_write_out(nbw_iterator_t *IW, void *data, int len);

void dump_buffers(netbuffer_t *H);

#endif
