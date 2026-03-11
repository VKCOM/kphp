import os
import shutil
import subprocess
import sys

from .kphp_builder import KphpBuilder
from .file_utils import error_can_be_ignored


class KphpRunOnce(KphpBuilder):
    K2_KPHP_TRACKED_BUILTINS_LIST = " ".join([
        "_exception_set_location",
        "_explode_tuple2",
        "_explode_tuple3",
        "_explode_tuple4",
        "_tmp_substr",
        "_tmp_trim",
        "abs",
        "acos",
        "addcslashes",
        "addslashes",
        "array_chunk",
        "array_column",
        "array_combine",
        "array_count_values",
        "array_diff",
        "array_diff_assoc",
        "array_diff_key",
        "array_fill",
        "array_fill_keys",
        "array_filter",
        "array_filter_by_key",
        "array_find",
        "array_first_key",
        "array_first_value",
        "array_flip",
        "array_intersect",
        "array_intersect_assoc",
        "array_intersect_key",
        "array_is_vector",
        "array_key_exists",
        "array_key_first",
        "array_key_last",
        "array_keys",
        "array_keys_as_ints",
        "array_keys_as_strings",
        "array_last_key",
        "array_last_value",
        "array_map",
        "array_merge",
        "array_merge_into",
        "array_merge_recursive",
        "array_merge_spread",
        "array_pad",
        "array_pop",
        "array_push",
        "array_rand",
        "array_reduce",
        "array_replace",
        "array_reserve",
        "array_reserve_from",
        "array_reserve_map_int_keys",
        "array_reserve_map_string_keys",
        "array_reserve_vector",
        "array_reverse",
        "array_search",
        "array_shift",
        "array_slice",
        "array_splice",
        "array_sum",
        "array_swap_int_keys",
        "array_unique",
        "array_unset",
        "array_unshift",
        "array_values",
        "arsort",
        "asort",
        "atan",
        "atan2",
        "base_convert",
        "base64_decode",
        "base64_encode",
        "basename",
        "bcadd",
        "bccomp",
        "bcdiv",
        "bcmod",
        "bcmul",
        "bcpow",
        "bcscale",
        "bcsqrt",
        "bcsub",
        "bin2hex",
        "bindec",
        "ceil",
        "checkdate",
        "chr",
        "CompileTimeLocation$$__construct",
        "CompileTimeLocation$$calculate",
        "confdata_get_value",
        "confdata_get_values_by_predefined_wildcard",
        "cos",
        "count",
        "count_chars",
        "crc32",
        "create_vector",
        "critical_error",
        "ctype_alnum",
        "ctype_digit",
        "ctype_xdigit",
        "curl_close",
        "curl_errno",
        "curl_error",
        "curl_exec",
        "curl_exec_concurrently",
        "curl_getinfo",
        "curl_init",
        "curl_multi_add_handle",
        "curl_multi_close",
        "curl_multi_errno",
        "curl_multi_exec",
        "curl_multi_getcontent",
        "curl_multi_info_read",
        "curl_multi_init",
        "curl_multi_remove_handle",
        "curl_multi_select",
        "curl_multi_setopt",
        "curl_multi_strerror",
        "curl_setopt",
        "curl_setopt_array",
        "date",
        "date_default_timezone_get",
        "date_default_timezone_set",
        "DateInterval$$__construct",
        "DateInterval$$createFromDateString",
        "DateInterval$$format",
        "DateTime$$__construct",
        "DateTime$$add",
        "DateTime$$createFromFormat",
        "DateTime$$diff",
        "DateTime$$format",
        "DateTime$$getTimestamp",
        "DateTime$$modify",
        "DateTime$$setDate",
        "DateTime$$setTime",
        "DateTime$$setTimestamp",
        "DateTimeImmutable$$__construct",
        "DateTimeImmutable$$add",
        "DateTimeImmutable$$createFromFormat",
        "DateTimeImmutable$$diff",
        "DateTimeImmutable$$format",
        "DateTimeImmutable$$getTimestamp",
        "DateTimeImmutable$$modify",
        "DateTimeImmutable$$setTime",
        "DateTimeImmutable$$setTimestamp",
        "DateTimeZone$$__construct",
        "debug_backtrace",
        "decbin",
        "dechex",
        "deflate_add",
        "deflate_init",
        "deg2rad",
        "die",
        "echo",
        "empty",
        "err",
        "error_get_last",
        "error_reporting",
        "Error$$getCode",
        "Error$$getFile",
        "Error$$getLine",
        "Error$$getMessage",
        "Error$$getTrace",
        "Error$$getTraceAsString",
        "estimate_memory_usage",
        "Exception$$__construct",
        "Exception$$getCode",
        "Exception$$getFile",
        "Exception$$getLine",
        "Exception$$getMessage",
        "Exception$$getTrace",
        "Exception$$getTraceAsString",
        "exit",
        "exp",
        "explode",
        "extract_kphp_rpc_response_extra_info",
        "fclose",
        "fetch_double",
        "fetch_int",
        "fetch_long",
        "fetch_string",
        "fflush",
        "file",
        "file_get_contents",
        "filesize",
        "floor",
        "fmod",
        "fopen",
        "fprintf",
        "fread",
        "function_exists",
        "fwrite",
        "get_class",
        "get_engine_version",
        "get_hash_of_class",
        "get_job_workers_number",
        "get_kphp_cluster_name",
        "get_running_fork_id",
        "getdate",
        "getKeyByPos",
        "gettype",
        "getValueByPos",
        "gmdate",
        "gmmktime",
        "gzcompress",
        "gzdecode",
        "gzencode",
        "gzinflate",
        "gzuncompress",
        "hash",
        "hash_equals",
        "hash_hmac",
        "header",
        "headers_list",
        "hex2bin",
        "hexdec",
        "hrtime",
        "html_entity_decode",
        "htmlentities",
        "htmlspecialchars",
        "htmlspecialchars_decode",
        "http_build_query",
        "iconv",
        "implode",
        "in_array",
        "inet_pton",
        "ini_get",
        "instance_cache_delete",
        "instance_cache_fetch",
        "instance_cache_store",
        "instance_cache_update_ttl",
        "instance_cast",
        "instance_deserialize",
        "instance_deserialize_safe",
        "instance_serialize",
        "instance_to_array",
        "ip2long",
        "ip2ulong",
        "is_array",
        "is_bool",
        "is_confdata_loaded",
        "is_double",
        "is_file",
        "is_finite",
        "is_float",
        "is_infinite",
        "is_int",
        "is_integer",
        "is_kphp_job_workers_enabled",
        "is_nan",
        "is_null",
        "is_numeric",
        "is_object",
        "is_scalar",
        "is_string",
        "json_decode",
        "json_encode",
        "JsonEncoder$$from_json_impl",
        "JsonEncoder$$getLastError",
        "JsonEncoder$$to_json_impl",
        "kml_get_custom_property",
        "kml_xgboost_predict_matrix",
        "kphp_get_runtime_config",
        "kphp_set_context_on_error",
        "KphpJobWorkerResponseError$$getError",
        "KphpJobWorkerResponseError$$getErrorCode",
        "KphpRpcRequestsExtraInfo$$get",
        "krsort",
        "ksort",
        "lcfirst",
        "lcg_value",
        "levenshtein",
        "likely",
        "log",
        "long2ip",
        "ltrim",
        "make_clone",
        "max",
        "mb_check_encoding",
        "mb_stripos",
        "mb_strlen",
        "mb_strpos",
        "mb_strtolower",
        "mb_strtoupper",
        "mb_substr",
        "md5",
        "memory_get_detailed_stats",
        "memory_get_peak_usage",
        "memory_get_total_usage",
        "memory_get_usage",
        "microtime",
        "min",
        "mktime",
        "msgpack_deserialize",
        "msgpack_serialize",
        "mt_getrandmax",
        "mt_rand",
        "mt_srand",
        "natsort",
        "nl2br",
        "numa_get_bound_node",
        "number_format",
        "ob_clean",
        "ob_end_clean",
        "ob_end_flush",
        "ob_get_clean",
        "ob_get_contents",
        "ob_get_length",
        "ob_get_level",
        "ob_start",
        "openssl_cipher_iv_length",
        "openssl_decrypt",
        "openssl_encrypt",
        "openssl_get_cipher_methods",
        "openssl_pkey_get_private",
        "openssl_pkey_get_public",
        "openssl_private_decrypt",
        "openssl_public_encrypt",
        "openssl_random_pseudo_bytes",
        "openssl_sign",
        "openssl_verify",
        "openssl_x509_parse",
        "ord",
        "pack",
        "parse_str",
        "parse_url",
        "php_sapi_name",
        "php_uname",
        "posix_getpid",
        "posix_getpwuid",
        "posix_getuid",
        "preg_match",
        "preg_match_all",
        "preg_quote",
        "preg_replace",
        "preg_replace_callback",
        "print",
        "print_r",
        "printf",
        "rand",
        "range",
        "rawurldecode",
        "rawurlencode",
        "realpath",
        "register_kphp_on_oom_callback",
        "register_kphp_on_warning_callback",
        "register_shutdown_function",
        "round",
        "rpc_clean",
        "rpc_parse",
        "rpc_queue_create",
        "rpc_queue_empty",
        "rpc_queue_next",
        "rpc_server_fetch_request",
        "rpc_server_store_response",
        "rpc_tl_pending_queries_count",
        "rpc_tl_query",
        "rpc_tl_query_result",
        "rpc_tl_query_result_synchronously",
        "rsort",
        "rtrim",
        "sched_yield",
        "sched_yield_sleep",
        "serialize",
        "set_detect_incorrect_encoding_names_warning",
        "set_fail_rpc_on_int32_overflow",
        "set_json_log_on_timeout_mode",
        "set_migration_php8_warning",
        "setlocale",
        "sha1",
        "shuffle",
        "similar_text",
        "sin",
        "sizeof",
        "sleep",
        "sort",
        "sprintf",
        "sqrt",
        "store_error",
        "store_int",
        "store_long",
        "store_string",
        "str_ends_with",
        "str_ireplace",
        "str_pad",
        "str_repeat",
        "str_replace",
        "str_split",
        "str_starts_with",
        "strcasecmp",
        "strcmp",
        "stream_socket_client",
        "strip_tags",
        "stripos",
        "stripslashes",
        "stristr",
        "strlen",
        "strnatcmp",
        "strncmp",
        "strpos",
        "strrev",
        "strripos",
        "strrpos",
        "strstr",
        "strtolower",
        "strtotime",
        "strtoupper",
        "strtr",
        "substr",
        "substr_compare",
        "substr_count",
        "substr_replace",
        "tan",
        "time",
        "to_array_debug",
        "trim",
        "typed_rpc_tl_query",
        "typed_rpc_tl_query_result",
        "typed_rpc_tl_query_result_synchronously",
        "uasort",
        "UberH3$$geoToH3",
        "UberH3$$h3ToParent",
        "ucfirst",
        "ucwords",
        "uksort",
        "uniqid",
        "unlink",
        "unpack",
        "unserialize",
        "urldecode",
        "urlencode",
        "usleep",
        "usort",
        "var_dump",
        "var_export",
        "vk_json_encode",
        "vk_json_encode_safe",
        "vk_sp_deunicode",
        "vk_sp_simplify",
        "vk_sp_to_lower",
        "vk_sp_to_upper",
        "vk_stats_hll_count",
        "vk_stats_hll_merge",
        "vk_utf8_to_win",
        "vk_win_to_utf8",
        "vsprintf",
        "wait",
        "wait_concurrently",
        "wait_multi",
        "wait_queue_create",
        "wait_queue_empty",
        "wait_queue_next",
        "wait_queue_push",
        "warning",
        "wordwrap",
        "zstd_compress",
        "zstd_uncompress",
    ])

    def __init__(self, php_script_path, artifacts_dir, working_dir, php_bin,
                 extra_include_dirs=None, vkext_dir=None, use_nocc=False, cxx_name="g++", k2_bin=None):
        super(KphpRunOnce, self).__init__(
            php_script_path=php_script_path,
            artifacts_dir=artifacts_dir,
            working_dir=working_dir,
            use_nocc=use_nocc,
            cxx_name=cxx_name,
        )

        self._php_stdout = None
        self._kphp_server_stdout = None
        self._php_tmp_dir = os.path.join(self._working_dir, "php")
        self._kphp_runtime_tmp_dir = os.path.join(self._working_dir, "kphp_runtime")
        if extra_include_dirs:
            self._include_dirs.extend(extra_include_dirs)
        self._vkext_dir = vkext_dir
        self._php_bin = php_bin
        self.k2_bin = k2_bin

    def _get_extensions(self):
        if sys.platform == "darwin":
            extensions = []
        else:
            extensions = [
                ("extension", "json.so"),
                ("extension", "bcmath.so"),
                ("extension", "iconv.so"),
                ("extension", "mbstring.so"),
                ("extension", "curl.so"),
                ("extension", "tokenizer.so"),
                ("extension", "h3.so"),
                ("extension", "zstd.so"),
                ("extension", "ctype.so")
            ]

        if self._vkext_dir:
            vkext_so = None
            if self._php_bin.endswith("php7.2"):
                vkext_so = os.path.join(self._vkext_dir, "modules7.2", "vkext.so")
            elif self._php_bin.endswith("php7.4"):
                vkext_so = os.path.join(self._vkext_dir, "modules7.4", "vkext.so")
            elif sys.platform == "darwin":
                vkext_so = os.path.join(self._vkext_dir, "modules", "vkext.dylib")

            if not vkext_so or not os.path.exists(vkext_so):
                vkext_so = "vkext.so"
            extensions.append(("extension", vkext_so))

        return extensions

    def run_with_php(self, extra_options=[], runs_cnt=1):
        self._clear_working_dir(self._php_tmp_dir)
        options = self._get_extensions()
        options.extend(extra_options)
        options.extend([
            ("display_errors", 0),
            ("log_errors", 1),
            ("memory_limit", "3072M"),
            ("xdebug.var_display_max_depth", -1),
            ("xdebug.var_display_max_children", -1),
            ("xdebug.var_display_max_data", -1),
            ("include_path", ":".join(self._include_dirs))
        ])

        cmd = [self._php_bin, "-n"]
        for k, v in options:
            cmd.append("-d {}='{}'".format(k, v))
        cmd.append(self._test_file_path)
        php_proc = subprocess.Popen(cmd, cwd=self._php_tmp_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self._php_stdout, php_stderr = self._wait_proc(php_proc)

        # We can assume that php is always idempotent and just copy output instead of running php script several times
        self._php_stdout *= runs_cnt

        if php_stderr:
            if 'GITHUB_ACTIONS' in os.environ:
                print("php_stderr: " + str(php_stderr))
            else:
                self._move_to_artifacts("php_stderr", php_proc.returncode, content=php_stderr)

        if not os.listdir(self._php_tmp_dir):
            shutil.rmtree(self._php_tmp_dir, ignore_errors=True)

        return php_proc.returncode == 0

    def run_with_kphp(self, runs_cnt=1, args=[]):
        if self.should_use_k2():
            return self.run_with_kphp_and_k2(runs_cnt=runs_cnt, args=args)
        else:
            return self.run_with_kphp_server(runs_cnt=runs_cnt, args=args)

    def run_with_kphp_server(self, runs_cnt=1, args=[]):
        self._clear_working_dir(self._kphp_runtime_tmp_dir)

        sanitizer_log_name = "kphp_runtime_sanitizer_log"
        env, sanitizer_glob_mask = self._prepare_sanitizer_env(self._kphp_runtime_tmp_dir, sanitizer_log_name)

        cmd = [self._kphp_runtime_bin, "--once={}".format(runs_cnt), "--profiler-log-prefix", "profiler.log",
               "--worker-queries-to-reload", "1"] + args
        if not os.getuid():
            cmd += ["-u", "root", "-g", "root"]
        kphp_server_proc = subprocess.Popen(cmd,
                                            cwd=self._kphp_runtime_tmp_dir,
                                            env=env,
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE)
        self._kphp_server_stdout, kphp_runtime_stderr = self._wait_proc(kphp_server_proc)

        self._move_sanitizer_logs_to_artifacts(sanitizer_glob_mask, kphp_server_proc, sanitizer_log_name)
        ignore_stderr = error_can_be_ignored(
            ignore_patterns=[
                "^\\[\\d+\\]\\[\\d{4}\\-\\d{2}\\-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d+ php\\-runner\\.cpp\\s+\\d+\\].+$"
            ],
            binary_error_text=kphp_runtime_stderr)

        if not ignore_stderr:
            self._kphp_runtime_stderr = self._move_to_artifacts(
                "kphp_runtime_stderr",
                kphp_server_proc.returncode,
                content=kphp_runtime_stderr)

        return kphp_server_proc.returncode == 0

    def run_with_kphp_and_k2(self, runs_cnt=1, args=[]):
        self._clear_working_dir(self._kphp_runtime_tmp_dir)

        k2_node_bin = self.k2_bin

        cmd = [k2_node_bin, "run-once", "--image", os.path.join(self._kphp_build_tmp_dir, "component.so"), "--runs-count={}".format(runs_cnt), "--crypto", "--instance-cache"] + args

        env = os.environ.copy()
        if "RUST_LOG" not in env:
            env["RUST_LOG"] = "Debug"

        k2_runtime_proc = subprocess.Popen(cmd,
                                           cwd=self._kphp_runtime_tmp_dir,
                                           env=env,
                                           stdout=subprocess.PIPE,
                                           stderr=subprocess.PIPE)

        self._kphp_server_stdout, _kphp_server_stderr = self._wait_proc(k2_runtime_proc)

        ignore_stderr = error_can_be_ignored(
            ignore_patterns=[".*DEBUG.*", ".*INFO.*"],
            binary_error_text=_kphp_server_stderr)

        if not ignore_stderr:
            self._kphp_runtime_stderr = self._move_to_artifacts(
                "k2_runtime_stderr",
                k2_runtime_proc.returncode,
                content=_kphp_server_stderr)
        return k2_runtime_proc.returncode == 0

    def compare_php_and_kphp_stdout(self):
        if self._kphp_server_stdout == self._php_stdout:
            return True

        diff_artifact = self._move_to_artifacts("php_vs_kphp.diff", 1, b"TODO")
        php_stdout_file = os.path.join(self._artifacts_dir, "php_stdout")
        with open(php_stdout_file, 'wb') as f:
            f.write(self._php_stdout)
        kphp_server_stdout_file = os.path.join(self._artifacts_dir, "kphp_server_stdout")
        with open(kphp_server_stdout_file, 'wb') as f:
            f.write(self._kphp_server_stdout)

        with open(diff_artifact.file, 'wb') as f:
            subprocess.call(["diff", "--text", "-ud", php_stdout_file, kphp_server_stdout_file], stdout=f)
            if 'GITHUB_ACTIONS' in os.environ:
                # just open and read the file - it's easier than messing with popen, etc.
                with open(diff_artifact.file, 'r') as f:
                    print('diff: ' + f.read())
        with open(diff_artifact.file, 'rb') as f:
            print(f.read())

        return False

    def should_use_k2(self):
        return self.k2_bin is not None
