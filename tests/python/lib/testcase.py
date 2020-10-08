import sys
import os
import shutil
import logging
import pathlib
import portalocker
import glob

from unittest import TestCase

from .kphp_server import KphpServer
from .kphp_builder import KphpBuilder
from .file_utils import search_combined_tlo, read_distcc_hosts, can_ignore_sanitizer_log

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
    test_suite_name, _ = os.path.splitext(os.path.basename(test_script_file))
    working_dir = os.path.join(test_script_dir, "_tmp/working_dir")
    artifacts_dir = os.path.join(test_script_dir, "_tmp/artifacts")
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
    kphp_server_working_dir = ""
    artifacts_dir = ""
    test_dir = ""

    @classmethod
    def _setup_tmp_folder(cls):
        script_file = sys.modules.get(cls.__module__).__file__
        cls.kphp_build_working_dir, cls.kphp_server_working_dir, cls.artifacts_dir, cls.test_dir = \
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

    def cmpAeq(self, value, delta):
        return self._Cmp(lambda x: abs(x - value) <= delta,
                         "abs(x - {}) <= {}".format(value, delta))


def _check_if_tl_required(php_dir):
    for directory, _, files in os.walk(php_dir):
        for file in files:
            if file.endswith(".php"):
                with open(os.path.join(directory, file)) as fp:
                    if "rpc_tl_query" in fp.read():
                        return True
    return False


class KphpServerAutoTestCase(BaseTestCase):
    """
    :type kphp_server: KphpServer
    """
    kphp_server = None
    kphp_server_bin = ""

    @classmethod
    def custom_setup(cls):
        if _check_if_tl_required(os.path.join(cls.test_dir, "php/")):
            os.environ["KPHP_GEN_TL_INTERNALS"] = "1"
            os.environ["KPHP_TL_SCHEMA"] = search_combined_tlo()

        cls.kphp_builder = KphpBuilder(
            php_script_path=os.path.join(cls.test_dir, "php/index.php"),
            artifacts_dir=cls.artifacts_dir,
            working_dir=cls.kphp_build_working_dir,
            distcc_hosts=read_distcc_hosts(os.environ.get("KPHP_TESTS_DISTCC_FILE", None))
        )

        kphp_build_lock_file = os.path.join(cls.kphp_build_working_dir, "kphp_build_lock")
        pathlib.Path(kphp_build_lock_file, exist_ok=True).touch()
        with open(kphp_build_lock_file, "r+") as lock:
            portalocker.lock(lock, portalocker.LOCK_EX)
            print("\nCompiling kphp server")
            # Специально выключаем KPHP_DYNAMIC_INCREMENTAL_LINKAGE, чтобы не было рейзов
            if not cls.kphp_builder.compile_with_kphp({"KPHP_DYNAMIC_INCREMENTAL_LINKAGE": "0"}):
                raise RuntimeError("Can't compile php script")
            cls.kphp_server_bin = os.path.join(cls.kphp_server_working_dir, "kphp_server")
            os.link(cls.kphp_builder.kphp_runtime_bin, cls.kphp_server_bin)

        cls.sanitizer_pattern = os.path.join(cls.kphp_server_working_dir, "engine_sanitizer_log")
        os.environ["ASAN_OPTIONS"] = "detect_leaks=0:log_path=" + cls.sanitizer_pattern
        os.environ["UBSAN_OPTIONS"] = "print_stacktrace=1:allow_addr2line=1:log_path={}".format(cls.sanitizer_pattern)
        cls.kphp_server = KphpServer(
            engine_bin=cls.kphp_server_bin,
            working_dir=cls.kphp_server_working_dir
        )
        cls.extra_class_setup()
        cls.kphp_server.start()

    @classmethod
    def custom_teardown(cls):
        cls.kphp_server.stop()
        cls.extra_class_teardown()
        try:
            os.remove(cls.kphp_server_bin)
        except OSError:
            pass
        for sanitizer_log in glob.glob(cls.sanitizer_pattern + ".*"):
            if not can_ignore_sanitizer_log(sanitizer_log):
                raise RuntimeError("Got unexpected sanitizer log '{}'".format(sanitizer_log))

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

    def assertKphpNoTerminatedRequests(self):
        """
        Проверяем что в статах kphp сервер нет terminated requests
        """
        self.assertEqual(
            self.kphp_server.get_stats(prefix="kphp_server.terminated_requests_"),
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
