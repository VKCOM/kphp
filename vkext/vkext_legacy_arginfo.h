/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: e6f9437f0343c15324ac916446bc499f7535ad89 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_hello_world, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_upcase, 0, 0, 1)
	ZEND_ARG_INFO(0, text)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_utf8_to_win, 0, 0, 1)
	ZEND_ARG_INFO(0, text)
	ZEND_ARG_INFO(0, max_len)
	ZEND_ARG_INFO(0, exit_on_error)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_win_to_utf8, 0, 0, 1)
	ZEND_ARG_INFO(0, text)
	ZEND_ARG_INFO(0, escape)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_flex, 0, 0, 5)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, case_name)
	ZEND_ARG_INFO(0, sex)
	ZEND_ARG_INFO(0, type)
	ZEND_ARG_INFO(0, lang_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_json_encode, 0, 0, 1)
	ZEND_ARG_INFO(0, val)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_whitespace_pack, 0, 0, 1)
	ZEND_ARG_INFO(0, text)
	ZEND_ARG_INFO(0, html_opt)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_new_rpc_connection, 0, 0, 2)
	ZEND_ARG_INFO(0, host_name)
	ZEND_ARG_INFO(0, port)
	ZEND_ARG_INFO(0, default_actor_id)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, connect_timeout)
	ZEND_ARG_INFO(0, reconnect_timeout)
ZEND_END_ARG_INFO()

#define arginfo_rpc_clean arginfo_vk_hello_world

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_send, 0, 0, 1)
	ZEND_ARG_INFO(0, rpc_connection)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

#define arginfo_rpc_send_noflush arginfo_rpc_send

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_flush, 0, 0, 0)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_get_and_parse, 0, 0, 1)
	ZEND_ARG_INFO(0, query_id)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

#define arginfo_rpc_get arginfo_rpc_get_and_parse

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_parse, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_set_fail_rpc_on_int32_overflow, 0, 0, 1)
	ZEND_ARG_INFO(0, fail_rpc)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_store_int, 0, 0, 1)
	ZEND_ARG_INFO(0, v)
ZEND_END_ARG_INFO()

#define arginfo_store_long arginfo_store_int

#define arginfo_store_string arginfo_store_int

#define arginfo_store_double arginfo_store_int

#define arginfo_store_float arginfo_store_int

ZEND_BEGIN_ARG_INFO_EX(arginfo_store_many, 0, 0, 0)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_store_header, 0, 0, 1)
	ZEND_ARG_INFO(0, actor_id)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

#define arginfo_fetch_int arginfo_vk_hello_world

#define arginfo_fetch_long arginfo_vk_hello_world

#define arginfo_fetch_double arginfo_vk_hello_world

#define arginfo_fetch_float arginfo_vk_hello_world

#define arginfo_fetch_string arginfo_vk_hello_world

#define arginfo_fetch_memcache_value arginfo_vk_hello_world

#define arginfo_fetch_end arginfo_vk_hello_world

#define arginfo_fetch_eof arginfo_vk_hello_world

#define arginfo_fetch_lookup_int arginfo_vk_hello_world

ZEND_BEGIN_ARG_INFO_EX(arginfo_fetch_lookup_data, 0, 0, 1)
	ZEND_ARG_INFO(0, x4_bytes_length)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_set_error_verbosity, 0, 0, 1)
	ZEND_ARG_INFO(0, verbosity)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_queue_create, 0, 0, 1)
	ZEND_ARG_INFO(0, request_ids)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_queue_empty, 0, 0, 1)
	ZEND_ARG_INFO(0, queue_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_queue_next, 0, 0, 1)
	ZEND_ARG_INFO(0, queue_id)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_queue_push, 0, 0, 2)
	ZEND_ARG_INFO(0, queue_id)
	ZEND_ARG_INFO(0, request_ids)
ZEND_END_ARG_INFO()

#define arginfo_vk_clear_stats arginfo_vk_hello_world

#define arginfo_rpc_tl_pending_queries_count arginfo_vk_hello_world

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_tl_query, 0, 0, 2)
	ZEND_ARG_INFO(0, rpc_connection)
	ZEND_ARG_INFO(0, tl_queries)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, ignore_answer)
	ZEND_ARG_INFO(0, requests_metrics)
	ZEND_ARG_INFO(0, need_responses_metrics)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_tl_query_one, 0, 0, 2)
	ZEND_ARG_INFO(0, rpc_connection)
	ZEND_ARG_INFO(0, tl_query)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_tl_query_result, 0, 0, 1)
	ZEND_ARG_INFO(0, request_ids)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpc_tl_query_result_one, 0, 0, 1)
	ZEND_ARG_INFO(0, request_id)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

#define arginfo_typed_rpc_tl_query arginfo_rpc_tl_query

#define arginfo_typed_rpc_tl_query_one arginfo_rpc_tl_query_one

#define arginfo_typed_rpc_tl_query_result arginfo_rpc_tl_query_result

#define arginfo_typed_rpc_tl_query_result_one arginfo_rpc_tl_query_result_one

#define arginfo_enable_internal_rpc_queries arginfo_vk_hello_world

#define arginfo_disable_internal_rpc_queries arginfo_vk_hello_world

#define arginfo_rpc_get_last_send_error arginfo_vk_hello_world

#define arginfo_vkext_prepare_stats arginfo_vk_hello_world

#define arginfo_vkext_full_version arginfo_vk_hello_world

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_sp_simplify, 0, 0, 1)
	ZEND_ARG_INFO(0, str)
ZEND_END_ARG_INFO()

#define arginfo_vk_sp_full_simplify arginfo_vk_sp_simplify

#define arginfo_vk_sp_deunicode arginfo_vk_sp_simplify

#define arginfo_vk_sp_to_upper arginfo_vk_sp_simplify

#define arginfo_vk_sp_to_lower arginfo_vk_sp_simplify

#define arginfo_vk_sp_sort arginfo_vk_sp_simplify

#define arginfo_vk_sp_remove_repeats arginfo_vk_sp_simplify

#define arginfo_vk_sp_to_cyrillic arginfo_vk_sp_simplify

#define arginfo_vk_sp_words_only arginfo_vk_sp_simplify

ZEND_BEGIN_ARG_INFO_EX(arginfo_tl_config_load_file, 0, 0, 1)
	ZEND_ARG_INFO(0, tl_config_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_stats_hll_create, 0, 0, 0)
	ZEND_ARG_INFO(0, arr)
	ZEND_ARG_INFO(0, size)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_stats_hll_add, 0, 0, 2)
	ZEND_ARG_INFO(0, hll)
	ZEND_ARG_INFO(0, arr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_stats_hll_merge, 0, 0, 1)
	ZEND_ARG_INFO(0, hlls)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_vk_stats_hll_count, 0, 0, 1)
	ZEND_ARG_INFO(0, hll)
ZEND_END_ARG_INFO()

#define arginfo_vk_stats_hll_pack arginfo_vk_stats_hll_count

#define arginfo_vk_stats_hll_unpack arginfo_vk_stats_hll_count

#define arginfo_vk_stats_hll_is_packed arginfo_vk_stats_hll_count


ZEND_FUNCTION(vk_hello_world);
ZEND_FUNCTION(vk_upcase);
ZEND_FUNCTION(vk_utf8_to_win);
ZEND_FUNCTION(vk_win_to_utf8);
ZEND_FUNCTION(vk_flex);
ZEND_FUNCTION(vk_json_encode);
ZEND_FUNCTION(vk_whitespace_pack);
ZEND_FUNCTION(new_rpc_connection);
ZEND_FUNCTION(rpc_clean);
ZEND_FUNCTION(rpc_send);
ZEND_FUNCTION(rpc_send_noflush);
ZEND_FUNCTION(rpc_flush);
ZEND_FUNCTION(rpc_get_and_parse);
ZEND_FUNCTION(rpc_get);
ZEND_FUNCTION(rpc_parse);
ZEND_FUNCTION(set_fail_rpc_on_int32_overflow);
ZEND_FUNCTION(store_int);
ZEND_FUNCTION(store_long);
ZEND_FUNCTION(store_string);
ZEND_FUNCTION(store_double);
ZEND_FUNCTION(store_float);
ZEND_FUNCTION(store_many);
ZEND_FUNCTION(store_header);
ZEND_FUNCTION(fetch_int);
ZEND_FUNCTION(fetch_long);
ZEND_FUNCTION(fetch_double);
ZEND_FUNCTION(fetch_float);
ZEND_FUNCTION(fetch_string);
ZEND_FUNCTION(fetch_memcache_value);
ZEND_FUNCTION(fetch_end);
ZEND_FUNCTION(fetch_eof);
ZEND_FUNCTION(fetch_lookup_int);
ZEND_FUNCTION(fetch_lookup_data);
ZEND_FUNCTION(vk_set_error_verbosity);
ZEND_FUNCTION(rpc_queue_create);
ZEND_FUNCTION(rpc_queue_empty);
ZEND_FUNCTION(rpc_queue_next);
ZEND_FUNCTION(rpc_queue_push);
ZEND_FUNCTION(vk_clear_stats);
ZEND_FUNCTION(rpc_tl_pending_queries_count);
ZEND_FUNCTION(rpc_tl_query);
ZEND_FUNCTION(rpc_tl_query_one);
ZEND_FUNCTION(rpc_tl_query_result);
ZEND_FUNCTION(rpc_tl_query_result_one);
ZEND_FUNCTION(typed_rpc_tl_query);
ZEND_FUNCTION(typed_rpc_tl_query_one);
ZEND_FUNCTION(typed_rpc_tl_query_result);
ZEND_FUNCTION(typed_rpc_tl_query_result_one);
ZEND_FUNCTION(enable_internal_rpc_queries);
ZEND_FUNCTION(disable_internal_rpc_queries);
ZEND_FUNCTION(rpc_get_last_send_error);
ZEND_FUNCTION(vkext_prepare_stats);
ZEND_FUNCTION(vkext_full_version);
ZEND_FUNCTION(vk_sp_simplify);
ZEND_FUNCTION(vk_sp_full_simplify);
ZEND_FUNCTION(vk_sp_deunicode);
ZEND_FUNCTION(vk_sp_to_upper);
ZEND_FUNCTION(vk_sp_to_lower);
ZEND_FUNCTION(vk_sp_sort);
ZEND_FUNCTION(vk_sp_remove_repeats);
ZEND_FUNCTION(vk_sp_to_cyrillic);
ZEND_FUNCTION(vk_sp_words_only);
ZEND_FUNCTION(tl_config_load_file);
ZEND_FUNCTION(vk_stats_hll_create);
ZEND_FUNCTION(vk_stats_hll_add);
ZEND_FUNCTION(vk_stats_hll_merge);
ZEND_FUNCTION(vk_stats_hll_count);
ZEND_FUNCTION(vk_stats_hll_pack);
ZEND_FUNCTION(vk_stats_hll_unpack);
ZEND_FUNCTION(vk_stats_hll_is_packed);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(vk_hello_world, arginfo_vk_hello_world)
	ZEND_FE(vk_upcase, arginfo_vk_upcase)
	ZEND_FE(vk_utf8_to_win, arginfo_vk_utf8_to_win)
	ZEND_FE(vk_win_to_utf8, arginfo_vk_win_to_utf8)
	ZEND_FE(vk_flex, arginfo_vk_flex)
	ZEND_FE(vk_json_encode, arginfo_vk_json_encode)
	ZEND_FE(vk_whitespace_pack, arginfo_vk_whitespace_pack)
	ZEND_FE(new_rpc_connection, arginfo_new_rpc_connection)
	ZEND_FE(rpc_clean, arginfo_rpc_clean)
	ZEND_FE(rpc_send, arginfo_rpc_send)
	ZEND_FE(rpc_send_noflush, arginfo_rpc_send_noflush)
	ZEND_FE(rpc_flush, arginfo_rpc_flush)
	ZEND_FE(rpc_get_and_parse, arginfo_rpc_get_and_parse)
	ZEND_FE(rpc_get, arginfo_rpc_get)
	ZEND_FE(rpc_parse, arginfo_rpc_parse)
	ZEND_FE(set_fail_rpc_on_int32_overflow, arginfo_set_fail_rpc_on_int32_overflow)
	ZEND_FE(store_int, arginfo_store_int)
	ZEND_FE(store_long, arginfo_store_long)
	ZEND_FE(store_string, arginfo_store_string)
	ZEND_FE(store_double, arginfo_store_double)
	ZEND_FE(store_float, arginfo_store_float)
	ZEND_FE(store_many, arginfo_store_many)
	ZEND_FE(store_header, arginfo_store_header)
	ZEND_FE(fetch_int, arginfo_fetch_int)
	ZEND_FE(fetch_long, arginfo_fetch_long)
	ZEND_FE(fetch_double, arginfo_fetch_double)
	ZEND_FE(fetch_float, arginfo_fetch_float)
	ZEND_FE(fetch_string, arginfo_fetch_string)
	ZEND_FE(fetch_memcache_value, arginfo_fetch_memcache_value)
	ZEND_FE(fetch_end, arginfo_fetch_end)
	ZEND_FE(fetch_eof, arginfo_fetch_eof)
	ZEND_FE(fetch_lookup_int, arginfo_fetch_lookup_int)
	ZEND_FE(fetch_lookup_data, arginfo_fetch_lookup_data)
	ZEND_FE(vk_set_error_verbosity, arginfo_vk_set_error_verbosity)
	ZEND_FE(rpc_queue_create, arginfo_rpc_queue_create)
	ZEND_FE(rpc_queue_empty, arginfo_rpc_queue_empty)
	ZEND_FE(rpc_queue_next, arginfo_rpc_queue_next)
	ZEND_FE(rpc_queue_push, arginfo_rpc_queue_push)
	ZEND_FE(vk_clear_stats, arginfo_vk_clear_stats)
	ZEND_FE(rpc_tl_pending_queries_count, arginfo_rpc_tl_pending_queries_count)
	ZEND_FE(rpc_tl_query, arginfo_rpc_tl_query)
	ZEND_FE(rpc_tl_query_one, arginfo_rpc_tl_query_one)
	ZEND_FE(rpc_tl_query_result, arginfo_rpc_tl_query_result)
	ZEND_FE(rpc_tl_query_result_one, arginfo_rpc_tl_query_result_one)
	ZEND_FE(typed_rpc_tl_query, arginfo_typed_rpc_tl_query)
	ZEND_FE(typed_rpc_tl_query_one, arginfo_typed_rpc_tl_query_one)
	ZEND_FE(typed_rpc_tl_query_result, arginfo_typed_rpc_tl_query_result)
	ZEND_FE(typed_rpc_tl_query_result_one, arginfo_typed_rpc_tl_query_result_one)
	ZEND_FE(enable_internal_rpc_queries, arginfo_enable_internal_rpc_queries)
	ZEND_FE(disable_internal_rpc_queries, arginfo_disable_internal_rpc_queries)
	ZEND_FE(rpc_get_last_send_error, arginfo_rpc_get_last_send_error)
	ZEND_FE(vkext_prepare_stats, arginfo_vkext_prepare_stats)
	ZEND_FE(vkext_full_version, arginfo_vkext_full_version)
	ZEND_FE(vk_sp_simplify, arginfo_vk_sp_simplify)
	ZEND_FE(vk_sp_full_simplify, arginfo_vk_sp_full_simplify)
	ZEND_FE(vk_sp_deunicode, arginfo_vk_sp_deunicode)
	ZEND_FE(vk_sp_to_upper, arginfo_vk_sp_to_upper)
	ZEND_FE(vk_sp_to_lower, arginfo_vk_sp_to_lower)
	ZEND_FE(vk_sp_sort, arginfo_vk_sp_sort)
	ZEND_FE(vk_sp_remove_repeats, arginfo_vk_sp_remove_repeats)
	ZEND_FE(vk_sp_to_cyrillic, arginfo_vk_sp_to_cyrillic)
	ZEND_FE(vk_sp_words_only, arginfo_vk_sp_words_only)
	ZEND_FE(tl_config_load_file, arginfo_tl_config_load_file)
	ZEND_FE(vk_stats_hll_create, arginfo_vk_stats_hll_create)
	ZEND_FE(vk_stats_hll_add, arginfo_vk_stats_hll_add)
	ZEND_FE(vk_stats_hll_merge, arginfo_vk_stats_hll_merge)
	ZEND_FE(vk_stats_hll_count, arginfo_vk_stats_hll_count)
	ZEND_FE(vk_stats_hll_pack, arginfo_vk_stats_hll_pack)
	ZEND_FE(vk_stats_hll_unpack, arginfo_vk_stats_hll_unpack)
	ZEND_FE(vk_stats_hll_is_packed, arginfo_vk_stats_hll_is_packed)
	ZEND_FE_END
};
