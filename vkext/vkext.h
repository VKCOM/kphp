// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_H__
#define __VKEXT_H__

#include "vkext/vk_zend.h"

#define VKEXT_VERSION "1.02"

#define VKEXT_NAME "vk_extension"

#define USE_ZEND_ALLOC 1

PHP_MINIT_FUNCTION (vkext);
PHP_RINIT_FUNCTION (vkext);
PHP_MSHUTDOWN_FUNCTION (vkext);
PHP_RSHUTDOWN_FUNCTION (vkext);

PHP_FUNCTION (vk_hello_world);
PHP_FUNCTION (vk_upcase);
PHP_FUNCTION (vk_utf8_to_win);
PHP_FUNCTION (vk_win_to_utf8);
PHP_FUNCTION (vk_flex);
PHP_FUNCTION (vk_json_encode);
PHP_FUNCTION (vk_whitespace_pack);
PHP_FUNCTION (rpc_clean);
PHP_FUNCTION (rpc_send);
PHP_FUNCTION (rpc_send_noflush);
PHP_FUNCTION (rpc_flush);
PHP_FUNCTION (rpc_get_and_parse);
PHP_FUNCTION (rpc_get);
PHP_FUNCTION (rpc_parse);
PHP_FUNCTION (new_rpc_connection);
PHP_FUNCTION (set_fail_rpc_on_int32_overflow);
PHP_FUNCTION (store_int);
PHP_FUNCTION (store_long);
PHP_FUNCTION (store_string);
PHP_FUNCTION (store_double);
PHP_FUNCTION (store_float);
PHP_FUNCTION (store_many);
PHP_FUNCTION (store_header);
PHP_FUNCTION (fetch_int);
PHP_FUNCTION (fetch_long);
PHP_FUNCTION (fetch_double);
PHP_FUNCTION (fetch_float);
PHP_FUNCTION (fetch_string);
PHP_FUNCTION (fetch_memcache_value);
PHP_FUNCTION (fetch_end);
PHP_FUNCTION (fetch_eof);
PHP_FUNCTION (fetch_lookup_int);
PHP_FUNCTION (fetch_lookup_data);
PHP_FUNCTION (vk_set_error_verbosity);
PHP_FUNCTION (rpc_queue_create);
PHP_FUNCTION (rpc_queue_empty);
PHP_FUNCTION (rpc_queue_next);
PHP_FUNCTION (rpc_queue_push);
PHP_FUNCTION (vk_clear_stats);
PHP_FUNCTION (rpc_tl_pending_queries_count);
PHP_FUNCTION (rpc_tl_query);
PHP_FUNCTION (rpc_tl_query_one);
PHP_FUNCTION (rpc_tl_query_result);
PHP_FUNCTION (rpc_tl_query_result_one);
PHP_FUNCTION (typed_rpc_tl_query);
PHP_FUNCTION (typed_rpc_tl_query_one);
PHP_FUNCTION (typed_rpc_tl_query_result);
PHP_FUNCTION (typed_rpc_tl_query_result_one);
PHP_FUNCTION (rpc_get_last_send_error);
PHP_FUNCTION (vkext_prepare_stats);
PHP_FUNCTION (vkext_full_version);
PHP_FUNCTION (tl_config_load);
PHP_FUNCTION (tl_config_load_file);
PHP_FUNCTION (enable_internal_rpc_queries);
PHP_FUNCTION (disable_internal_rpc_queries);

#define PHP_WARNING(t) (NULL TSRMLS_CC, E_WARNING, t);
#define PHP_ERROR(t) (NULL TSRMLS_CC, E_ERROR, t);
#define PHP_NOTICE(t) (NULL TSRMLS_CC, E_NOTICE, t);

extern zend_module_entry vkext_module_entry;
#define phpext_vkext_ptr &vkext_module_ptr

void write_buff(const char *s, int l);
void write_buff_char(char c);
void write_buff_long(long x);
void write_buff_double(double x);
void print_backtrace();
#endif
