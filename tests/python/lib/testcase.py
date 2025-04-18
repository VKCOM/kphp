import sys
import os
import shutil
import logging
import pathlib
import portalocker
import glob

from unittest import TestCase

from .kphp_server import KphpServer
from .k2_server import K2Server
from .kphp_builder import KphpBuilder
from .kphp_run_once import KphpRunOnce
from .file_utils import search_combined_tlo, can_ignore_sanitizer_log, search_php_bin, search_k2_bin
from .nocc_for_kphp_tester import nocc_env, nocc_start_daemon_in_background
from .web_server import WebServer

logging.disable(logging.DEBUG)


def _sync_data(tmp_dir, test_parent_dir):
    data_dir = os.path.join(test_parent_dir, "php/data")
    tmp_data_dir = os.path.join(tmp_dir, "data")

    if os.path.isdir(data_dir):
        os.makedirs(tmp_data_dir, exist_ok=True)
        for data_file in os.listdir(data_dir):
            full_tmp_file = os.path.join(tmp_data_dir, data_file)
            if os.path.exists(full_tmp_file):
                continue

            full_data_file = os.path.join(data_dir, data_file)
            if os.path.isfile(full_data_file):
                shutil.copy(full_data_file, tmp_data_dir)
            elif os.path.isdir(full_data_file):
                shutil.copytree(full_data_file, full_tmp_file)


def _get_tmp_folder_path(test_script_file):
    test_script_dir = os.path.dirname(os.path.realpath(test_script_file))
    tests_root_dir = test_script_dir
    while not tests_root_dir.endswith("python/tests"):
        tests_root_dir = os.path.dirname(tests_root_dir)
        if "python/tests" not in tests_root_dir:
            raise RuntimeError("Can't find tests root dir")

    python_tests_dir = os.path.dirname(tests_root_dir)
    tmp_dir = os.path.join(python_tests_dir, "_tmp/", test_script_dir[len(tests_root_dir) + 1:])
    test_suite_name, _ = os.path.splitext(os.path.basename(test_script_file))
    working_dir = os.path.join(tmp_dir, "working_dir")
    artifacts_dir = os.path.join(tmp_dir, "artifacts")
    tmp_dir_root = os.path.join(artifacts_dir, "tmp_{}".format(test_suite_name))
    return working_dir, tmp_dir_root, artifacts_dir, test_script_dir


def _make_test_tmp_dir(tmp_dir_root):
    all_dirs = next(os.walk(tmp_dir_root))[1]
    ppid = str(os.getppid())
    for tmp_dir in all_dirs:
        if tmp_dir != ppid:
            shutil.rmtree(os.path.join(tmp_dir_root, tmp_dir), ignore_errors=True)

    current_tmp_dir = os.path.join(tmp_dir_root, ppid)
    os.makedirs(current_tmp_dir, exist_ok=True)
    test_tmp_dir = os.path.join(current_tmp_dir, str(os.getpid()))
    if os.path.isdir(test_tmp_dir):
        shutil.rmtree(test_tmp_dir)
    os.makedirs(test_tmp_dir)
    return test_tmp_dir


def _create_tmp_folders(test_script_file):
    kphp_build_working_dir, tmp_dir_root, artifacts_dir, test_script_dir = _get_tmp_folder_path(test_script_file)
    for test_dir in (kphp_build_working_dir, tmp_dir_root, artifacts_dir):
        os.makedirs(test_dir, exist_ok=True)

    kphp_server_working_dir = _make_test_tmp_dir(tmp_dir_root)
    _sync_data(kphp_server_working_dir, test_script_dir)
    return kphp_build_working_dir, kphp_server_working_dir, artifacts_dir, test_script_dir


class BaseTestCase(TestCase):
    kphp_build_working_dir = ""
    web_server_working_dir = ""
    artifacts_dir = ""
    test_dir = ""

    @classmethod
    def _setup_tmp_folder(cls):
        script_file = sys.modules.get(cls.__module__).__file__
        cls.kphp_build_working_dir, cls.web_server_working_dir, cls.artifacts_dir, cls.test_dir = \
            _create_tmp_folders(script_file)

    @classmethod
    def setup_class(cls):
        cls._setup_tmp_folder()
        cls.custom_setup()

    @classmethod
    def teardown_class(cls):
        cls.custom_teardown()

    @classmethod
    def custom_setup(cls):
        pass

    @classmethod
    def custom_teardown(cls):
        pass

    def setup_method(self, method):
        self.custom_setup_method(method)

    def teardown_method(self, method):
        self.custom_teardown_method(method)

    def custom_setup_method(self, method):
        pass

    def custom_teardown_method(self, method):
        pass

    class _Cmp:
        def __init__(self, comparator, representation):
            self._comparator = comparator
            self._representation = representation

        def __eq__(self, x):
            return self._comparator(x)

        def __ne__(self, x):
            return not self._comparator(x)

        def __str__(self):
            return self._representation

    def cmpEq(self, value):
        return self._Cmp(lambda x: x == value, "x == {}".format(value))

    def cmpLt(self, value):
        return self._Cmp(lambda x: x < value, "x < {}".format(value))

    def cmpLe(self, value):
        return self._Cmp(lambda x: x <= value, "x <= {}".format(value))

    def cmpGt(self, value):
        return self._Cmp(lambda x: x > value, "x > {}".format(value))

    def cmpGe(self, value):
        return self._Cmp(lambda x: x >= value, "x >= {}".format(value))

    def cmpGeAndLe(self, ge, le):
        return self._Cmp(lambda x: ge <= x <= le, "{} <= x <= {}".format(ge, le))

    def cmpAeq(self, value, delta):
        return self._Cmp(lambda x: abs(x - value) <= delta,
                         "abs(x - {}) <= {}".format(value, delta))


def _check_if_tl_required(php_dir):
    for directory, _, files in os.walk(php_dir):
        for file in files:
            if file.endswith(".php"):
                with open(os.path.join(directory, file)) as fp:
                    content = fp.read()
                    if any(fun in content
                           for fun in ["rpc_tl_query", "rpc_server_fetch_request", "rpc_server_store_response"]):
                        return True
    return False


class WebServerAutoTestCase(BaseTestCase):
    """
    :type web_server: WebServer
    """
    web_server = None
    web_server_bin = ""
    kphp_builder = None
    sanitizer_pattern = None

    @classmethod
    def custom_setup(cls):
        if cls.should_use_nocc():
            nocc_start_daemon_in_background()

        cls.kphp_builder = KphpBuilder(
            php_script_path=os.path.join(cls.test_dir, "php/index.php"),
            artifacts_dir=cls.artifacts_dir,
            working_dir=cls.kphp_build_working_dir,
            use_nocc=cls.should_use_nocc(),
        )

        tl_schema_required = _check_if_tl_required(os.path.join(cls.test_dir, "php/"))
        kphp_build_lock_file = os.path.join(cls.kphp_build_working_dir, "kphp_build_lock")
        pathlib.Path(kphp_build_lock_file, exist_ok=True).touch()
        with open(kphp_build_lock_file, "r+") as lock:
            portalocker.lock(lock, portalocker.LOCK_EX)
            kphp_env = {
                # disable KPHP_DYNAMIC_INCREMENTAL_LINKAGE to avoid compilation races
                "KPHP_DYNAMIC_INCREMENTAL_LINKAGE": "0",
                "KPHP_VERBOSITY": "3",
            }
            if tl_schema_required:
                kphp_env["KPHP_GEN_TL_INTERNALS"] = "1"
                kphp_env["KPHP_TL_SCHEMA"] = search_combined_tlo(cls.kphp_build_working_dir)

            if cls.should_use_k2():
                kphp_env.update(cls.k2_server_env())

            print("\nCompiling kphp")
            if not cls.kphp_builder.compile_with_kphp(kphp_env):
                raise RuntimeError("Can't compile php script")

            if cls.should_use_k2():
                cls.web_server_bin = os.path.abspath(search_k2_bin())
            else:
                cls.web_server_bin = os.path.join(cls.web_server_working_dir, "kphp_server")
                os.link(cls.kphp_builder.kphp_runtime_bin, cls.web_server_bin)

        cls.sanitizer_pattern = os.path.join(cls.web_server_working_dir, "engine_sanitizer_log")
        os.environ["ASAN_OPTIONS"] = "log_path=" + cls.sanitizer_pattern
        os.environ["UBSAN_OPTIONS"] = f"print_stacktrace=1:allow_addr2line=1:log_path={cls.sanitizer_pattern}"

        if cls.should_use_k2():
            cls.web_server = K2Server(
                k2_server_bin=cls.web_server_bin,
                working_dir=cls.web_server_working_dir,
                kphp_build_dir=cls.kphp_builder.kphp_build_tmp_dir)
        else:
            cls.web_server = KphpServer(
                kphp_server_bin=cls.web_server_bin,
                working_dir=cls.web_server_working_dir)

        cls.extra_class_setup()
        print("\nStarting web-server")
        cls.web_server.start()

    @classmethod
    def custom_teardown(cls):
        cls.web_server.stop()
        cls.extra_class_teardown()
        try:
            os.remove(cls.web_server_bin)
        except OSError:
            pass
        for sanitizer_log in glob.glob(cls.sanitizer_pattern + ".*"):
            if not can_ignore_sanitizer_log(sanitizer_log):
                raise RuntimeError("Got unexpected sanitizer log '{}'".format(sanitizer_log))

    def custom_setup_method(self, method):
        pass

    def custom_teardown_method(self, method):
        self.web_server._engine_logs = []

    @classmethod
    def extra_class_setup(cls):
        """
        Можно переопределять в тест кейсе.
        Этот метод вызывается перед запуском тестов описанных в кейсе.
        """
        pass

    @classmethod
    def extra_class_teardown(cls):
        """
        Можно переопределять в тест кейсе.
        Этот метод вызывается после завершения всех тестов описанных в кейсе.
        """
        pass

    @classmethod
    def should_use_nocc(cls):
        return nocc_env("NOCC_SERVERS_FILENAME", None) is not None

    @classmethod
    def should_use_k2(cls):
        return search_k2_bin() is not None

    @classmethod
    def k2_server_env(cls):
        env = {"KPHP_MODE": "k2-server", "KPHP_ENABLE_FULL_PERFORMANCE_ANALYZE": "0",
               "KPHP_PROFILER": "0", "KPHP_USER_BINARY_PATH": "component.so", "KPHP_FORCE_LINK_RUNTIME": "1"}
        return env

    def assertKphpNoTerminatedRequests(self):
        """
        Проверяем что в статах kphp сервер нет terminated requests
        """
        self.assertEqual(
            self.web_server.get_stats(prefix="kphp_server.workers_general_errors_"),
            {
                "memory_limit_exceeded": 0,
                "timeout": 0,
                "exception": 0,
                "stack_overflow": 0,
                "php_assert": 0,
                "http_connection_close": 0,
                "rpc_connection_close": 0,
                "net_event_error": 0,
                "post_data_loading_error": 0,
                "unclassified": 0
            })


class KphpCompilerAutoTestCase(BaseTestCase):
    once_runner_trash_bin = []

    def __init__(self, method_name):
        super().__init__(method_name)
        self.php_version = "php7.4"

    @classmethod
    def extra_class_setup(cls):
        """
        Можно переопределять в тест кейсе.
        Этот метод вызывается перед запуском тестов описанных в кейсе.
        """
        pass

    @classmethod
    def extra_class_teardown(cls):
        """
        Можно переопределять в тест кейсе.
        Этот метод вызывается после завершения всех тестов описанных в кейсе.
        """
        pass

    @classmethod
    def custom_setup(cls):
        cls.kphp_build_working_dir = _make_test_tmp_dir(cls.kphp_build_working_dir)
        cls.extra_class_setup()

    @classmethod
    def custom_teardown(cls):
        cls.extra_class_teardown()
        for once_runner in cls.once_runner_trash_bin:
            once_runner.try_remove_kphp_build_trash()

    @classmethod
    def should_use_nocc(cls):
        return nocc_env("NOCC_SERVERS_FILENAME", None) is not None

    @classmethod
    def should_use_k2(cls):
        return search_k2_bin() is not None

    @classmethod
    def k2_cli_env(cls):
        env = {"KPHP_MODE": "k2-cli", "KPHP_ENABLE_FULL_PERFORMANCE_ANALYZE": "0",
               "KPHP_PROFILER": "0", "KPHP_USER_BINARY_PATH": "component.so", "KPHP_FORCE_LINK_RUNTIME": "1"}
        return env

    def make_kphp_once_runner(self, php_script_path):
        once_runner = KphpRunOnce(
            php_script_path=os.path.join(self.test_dir, php_script_path),
            artifacts_dir=self.web_server_working_dir,
            working_dir=self.kphp_build_working_dir,
            php_bin=search_php_bin(php_version=self.php_version),
            use_nocc=self.should_use_nocc(),
            k2_bin=os.path.abspath(search_k2_bin()) if self.should_use_k2() else None,
        )
        self.once_runner_trash_bin.append(once_runner)
        return once_runner

    def build_and_compare_with_php(self, php_script_path, kphp_env=None):
        if self.should_use_k2():
            kphp_env = kphp_env or {}
            kphp_env.update(self.k2_cli_env())
        once_runner = self.make_kphp_once_runner(php_script_path)
        self.assertTrue(once_runner.run_with_php(), "Got PHP error")
        self.assertTrue(once_runner.compile_with_kphp(kphp_env), "Got KPHP build error")
        self.assertTrue(once_runner.run_with_kphp(), "Got KPHP runtime error")
        self.assertTrue(once_runner.compare_php_and_kphp_stdout(), "Got PHP and KPHP diff")
        return once_runner
