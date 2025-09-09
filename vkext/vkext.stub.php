<?php
/**
 * ! THIS FILE CONTAINS STUBS ONLY, MUST NOT BE USED AT RUNTIME !
 *
 * @generate-function-entries
 * @generate-legacy-arginfo
 * This annotation is needed for vkext_arginfo.h generating. Use CMake target vkext_arginfo for it.
 */

function vk_hello_world() : string {
  return "";
}

function vk_upcase(string $text) : string {
  return "";
}

function vk_utf8_to_win(string $text, int $max_len = 0, bool $exit_on_error = false) : string {
  return "";
}

function vk_win_to_utf8(string$text, bool $escape = true) : string {
  return "";
}

function vk_flex(string $name, string $case_name, int $sex, string $type, int $lang_id) : string {
  return "";
}

/**
 * @param mixed $val
 * @return string|false 
 */
function vk_json_encode($val) {
  return "";
}


function vk_whitespace_pack(string $text, int $html_opt = 0) : string {
  return "";
}

/**
 * @param string $host_name
 * @param mixed $port
 * @param mixed $default_actor_id
 * @param float $timeout
 * @param mixed $connect_timeout
 * @param mixed $reconnect_timeout
 * @return int|null
 */
function new_rpc_connection($host_name, $port, $default_actor_id = 0, $timeout = 0.3, $connect_timeout = 0.3, $reconnect_timeout = 17.0) {
  return 0;
}

function rpc_clean() : void {
  return;
}

/**
 * @param int $rpc_connection
 * @param float $timeout
 * @return int|false
 */
function rpc_send($rpc_connection, $timeout = -1.0) {
  return 0;
}

/**
 * @param int $rpc_connection
 * @param float $timeout
 * @return int|false
 */
function rpc_send_noflush($rpc_connection, $timeout = -1.0) {
  return 0;
}

function rpc_flush(float $timeout = 0.3) : bool {
  return false;
}

function rpc_get_and_parse(mixed $query_id, float $timeout = -1.0) : bool {
  return false;
}

/**
 * @return string|false
 */
function rpc_get(mixed $query_id, float $timeout = -1.0) {
  return "";
}

function rpc_parse(mxied $data) : bool {
  return false;
}

function set_fail_rpc_on_int32_overflow(bool $fail_rpc) : bool {
  return false;
}

function store_int(mixed $v) : bool {
  return false;
}

function store_byte(mixed $v) : bool {
  return false;
}

function store_long(mixed $v) : bool {
  return false;
}

function store_string(mixed $v) : bool {
  return false;
}

function store_double(mixed $v) : bool {
  return false;
}

function store_float(mixed $v) : bool {
  return false;
}

function store_many(mixed ...$args) : bool {
  return false;
}

function store_header(mixed $actor_id, int $flags = 0) : bool {
  return false;
}

/**
 * @return int|false
 */
function fetch_int() {
  return 0;
}

/**
 * @return int|false
 */
function fetch_byte() {
  return 0;
}

/**
 * @return int|false
 */
function fetch_long() {
  return 0;
}

/**
 * @return float|false
 */
function fetch_double() {
  return 0.0;
}

/**
 * @return float|false
 */
function fetch_float() {
  return 0.0;
}

/**
 * @return string|false
 */
function fetch_string() {
  return "";
}

function fetch_memcache_value() : mixed {
  return 0xed;
}

function fetch_end() : bool {
  return false;
}

function fetch_eof() : bool {
  return false;
}

/**
 * @return int|false
 */
function fetch_lookup_int() {
  return 0;
}

/**
 * @return string|false
 */
function fetch_lookup_data(int $x4_bytes_length) {
  return "";
}

function vk_set_error_verbosity(int $verbosity) : bool {
  return false;
}

/**
 * @return int|false
 */
function rpc_queue_create(mixed $request_ids) {
  return 0;
}

function rpc_queue_empty(int $queue_id) : bool {
  return false;
}

/**
 * @return int|false
 */
function rpc_queue_next(int $queue_id, float $timeout = -1.0) {
  return 0;
}

function rpc_queue_push(int $queue_id, mixed $request_ids) : bool {
  return false;
}

function vk_clear_stats() : void {
  return;
}

function rpc_tl_pending_queries_count() : int {
  return 0;
}

function rpc_tl_query(int $rpc_connection, array $tl_queries, float $timeout = -1.0,
                      bool $ignore_answer = false, \KphpRpcRequestsExtraInfo $requests_extra_info = null,
                      bool $need_responses_extra_info = false) : array {
  return [];
}

/**
 * @return int|false
 */
function rpc_tl_query_one(int $rpc_connection, array $tl_query, float $timeout = -1.0) {
  return 0;
}

/**
 * @return array|false
 */
function rpc_tl_query_result(array $request_ids, float $timeout = -1.0) {
  return [];
}

/**
 * @return array|false
 */
function rpc_tl_query_result_one(int $request_id, float $timeout = -1.0) {
  return [];
}

function typed_rpc_tl_query(int $rpc_connection, array $tl_queries, float $timeout = -1.0,
                            bool $ignore_answer = false, \KphpRpcRequestsExtraInfo $requests_extra_info = null,
                            bool $need_responses_extra_info = false) : array {
  return [];
}

/**
 * @return int|false
 */
function typed_rpc_tl_query_one(int $rpc_connection, object $tl_query, float $timeout = -1.0) {
  return 0;
}

/**
 * @return array|false
 */
function typed_rpc_tl_query_result(array $request_ids, float $timeout = -1.0) {
  return [];
}

/**
 * @return object|false
 */
function typed_rpc_tl_query_result_one(int $request_id, float $timeout = -1.0) {
  return (object)[];
}

function enable_internal_rpc_queries() : void {
  return;
}

function disable_internal_rpc_queries() : void {
  return;
}

function rpc_get_last_send_error() : array {
  return [];
}

function vkext_prepare_stats() : string {
  return "";
}

function vkext_full_version() : string {
  return "";
}

function vk_sp_simplify(string $str) : string {
  return "";
}

function vk_sp_full_simplify(string $str) : string {
  return "";
}

function vk_sp_deunicode(string $str) : string {
  return "";
}

function vk_sp_to_upper(string $str) : string {
  return "";
}

function vk_sp_to_lower(string $str) : string {
  return "";
}

function vk_sp_sort(string $str) : string {
  return "";
}

function vk_sp_remove_repeats(string $str) : string {
  return "";
}

function vk_sp_to_cyrillic(string $str) : string {
  return "";
}

function vk_sp_words_only(string $str) : string {
  return "";
}

function tl_config_load_file(mixed $tl_config_path) : bool {
  return false;
}

/**
 * @return string|false
 */
function vk_stats_hll_create(array $arr = [], int $size = 256) {
  return "";
}

/**
 * @return string|false
 */
function vk_stats_hll_add(string $hll, array $arr) {
  return "";
}

/**
 * @return string|false
 */
function vk_stats_hll_merge(array $hlls) {
  return "";
}

/**
 * @return float|false
 */
function vk_stats_hll_count(string $hll) {
  return 0.0;
}

/**
 * @return string|false
 */
function vk_stats_hll_pack(string $hll) {
  return "";
}

/**
 * @return string|false
 */
function vk_stats_hll_unpack(string $hll) {
  return "";
}

function vk_stats_hll_is_packed(string $hll) : bool {
  return false;
}
