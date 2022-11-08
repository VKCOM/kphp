// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef	__NET_HTTP_SERVER__
#define	__NET_HTTP_SERVER__

#include <sys/cdefs.h>

#include "net/net-connections.h"

#define	MAX_HTTP_HEADER_SIZE (64 * 1024)
#define	MAX_HTTP_HEADER_QUERY_WORD_SIZE (16 * 1024)
#define	MAX_HTTP_HEADER_KEY_SIZE (4 * 1024)

struct http_server_functions {
  void *info;
  int (*execute)(struct connection *c, int op);		/* invoked from parse_execute() */
  int (*ht_wakeup)(struct connection *c);
  int (*ht_alarm)(struct connection *c);
  int (*ht_close)(struct connection *c, int who);
};

#define	HTTP_V09	9
#define	HTTP_V10	0x100
#define	HTTP_V11	0x101

/* in conn->custom_data, 104 bytes */
struct hts_data {
  int query_type;
  int query_flags;
  int query_words;
  int header_size;
  int first_line_size;
  int data_size;
  int host_offset;
  int host_size;
  int uri_offset;
  int uri_size;
  int http_ver;
  int wlen;
  char word[16];
  void *extra;
  int extra_int;
  int extra_int2;
  int extra_int3;
  int extra_int4;
  double extra_double, extra_double2;
};

/* for hts_data.query_type */
enum hts_query_type {
  htqt_none,
  htqt_head,
  htqt_get,
  htqt_post,
  htqt_options,
  htqt_error,
  htqt_empty
};

#define	QF_ERROR	1
#define QF_HOST		2
#define QF_DATASIZE	4
#define	QF_CONNECTION	8
#define	QF_KEEPALIVE	0x100
#define	QF_EXTRA_HEADERS	0x200

#define	HTS_DATA(c)	((struct hts_data *) ((c)->custom_data))
#define	HTS_FUNC(c)	((struct http_server_functions *) ((c)->extra))

extern conn_type_t ct_http_server;
extern struct http_server_functions default_http_server;

int hts_do_wakeup (struct connection *c);
int hts_parse_execute (struct connection *c);
int hts_std_wakeup (struct connection *c);
int hts_std_alarm (struct connection *c);
int hts_init_accepted (struct connection *c);
int hts_close_connection (struct connection *c, int who);

extern int http_connections;
extern long long http_queries, http_bad_headers, http_queries_size;

/* useful functions */
int get_http_header (const char *qHeaders, const int qHeadersLen, char *buffer, int b_len, const char *arg_name, const int arg_len);

void gen_http_date (char date_buffer[29], int time);
int gen_http_time (char *date_buffer, int *time);
char *cur_http_date ();
int write_basic_http_header (struct connection *c, int code, int date, int len, const char *add_header, const char *content_type);
int write_http_error (struct connection *c, int code);
int format_http_error_page(int code, char *buff, size_t buff_size);

/* END */
#endif
