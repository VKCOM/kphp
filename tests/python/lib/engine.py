import atexit
import os

import psutil
import re
import subprocess
import signal
import time
import json

from .colors import cyan
from .stats_receiver import StatsReceiver, StatsType
from .port_generator import get_port
from .tl_client import send_rpc_request


class Engine:
    def __init__(self, engine_bin, working_dir, options=None):
        """
        :param engine_bin: Путь до бинарника движка
        :param working_dir: Рабочая папка где будет запущен движек
        :param options: Словарь с дополнительными опциями, которые будут использоваться при запуске движка
            Специальная значения:
                option: True - передавать опцию без значения
                option: None - удалить дефолтно проставляемую опцию
        """
        self._engine_bin = engine_bin
        self._working_dir = working_dir
        self._engine_name = os.path.basename(engine_bin).replace('-', '_')
        self._log_file = os.path.join(working_dir, self._engine_name + ".log")
        self._stats_receiver = StatsReceiver(self._engine_name, working_dir, StatsType.STATSD)
        self._rpc_port = get_port()
        self._options = {
            "--log": self._log_file,
            "--statsd-port": self._stats_receiver.port,
            "--port": self._rpc_port,
            "--allow-loopback": True,
            "--dump-next-queries": 100000,
        }
        if not os.getuid():
            self._options["--user"] = "root"
            self._options["--group"] = "root"

        self._engine_process = None
        self._log_file_write_fd = None
        self._log_file_read_fd = None
        self._engine_logs = []
        self._binlog_path = None
        self._ignore_log_errors = False
        if options:
            self.update_options(options)

    def __del__(self):
        self.stop()

    @property
    def rpc_port(self):
        """
        :return: rpc порт, который слушает движок
        """
        return self._rpc_port

    @property
    def binlog_path(self):
        """
        :return: Путь до бинглога
        """
        return self._binlog_path

    @property
    def pid(self):
        """
        :return: pid запущенного процесса
        """
        return self._engine_process.pid

    def update_options(self, options):
        """
        Обновить опции движка
        :param options: Словарь с доп опциями
            Специальные значения:
                option: True - передавать опцию без значения
                option: None - удалить дефолтно проставляемую опцию
        """
        self._options.update(options)

    def ignore_log_errors(self):
        self._ignore_log_errors = True

    def create_binlog(self):
        """
        Создать пустой бинлог в рабочей директории двжика
        """
        binlog_path = self._engine_name + ".binlog"
        print("\nCreating binlog [{}]".format(cyan(binlog_path)))
        with open(self._log_file, 'ab') as logfile:
            cmd = [self._engine_bin, "--create-binlog", "0,1", binlog_path]
            if not os.getuid():
                cmd += ["--user", "root", "--group", "root"]
            ret_code = subprocess.call(
                cmd,
                stdout=logfile,
                stderr=subprocess.STDOUT,
                cwd=self._working_dir
            )
        if ret_code:
            raise RuntimeError("Can't create binlog")
        self._binlog_path = binlog_path

    def start(self, start_msgs=None):
        """
        Запустить движок
        :param start_msgs: Сообщение в логе, которое нужно проверить после запуска движка
        """
        self._stats_receiver.start()

        cmd = [self._engine_bin]
        for option, value_raw in self._options.items():
            if value_raw is not None:
                value_list = value_raw
                if type(value_raw) is not list:
                    value_list = [value_raw]
                for val in value_list:
                    cmd.append(option)
                    if val is not True:
                        cmd.append(str(val))

        if self._binlog_path:
            cmd.append(self._binlog_path)

        self._engine_logs = []
        self._log_file_write_fd = open(self._log_file, 'ab')
        self._log_file_read_fd = open(self._log_file, 'r')
        print("\nStarting engine: [{}]".format(cyan(" ".join(cmd))))

        # some of engines are leaky
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = "detect_leaks=0"
        self._engine_process = psutil.Popen(
            cmd,
            stdout=self._log_file_write_fd,
            stderr=subprocess.STDOUT,
            cwd=self._working_dir,
            env=env
        )

        if os.waitpid(self._engine_process.pid, os.WNOHANG)[1] != 0:
            raise RuntimeError("Can't start the engine process")

        self.assert_log(["Connected to statsd"], message="Can't find statsd connection message")
        self._stats_receiver.wait_next_stats()
        if start_msgs:
            self.assert_log(start_msgs, message="Engine expected start message cannot be found!")
        elif not self._ignore_log_errors:
            self._assert_no_errors()

        self._assert_availability()
        atexit.register(self.stop)

    def stop(self):
        """
        Остановить движок и проверить, что все в порядке
        """
        if self._engine_process is None or not self._engine_process.is_running():
            return

        self._assert_availability()
        print("\nStopping engine: [{}]".format(cyan(self._engine_bin)))
        self.send_signal(signal.SIGTERM)

        engine_stopped_properly, status = self.wait_termination(30)

        if not self._ignore_log_errors:
            self._assert_no_errors()
        self._log_file_read_fd.close()
        self._log_file_write_fd.close()
        self._stats_receiver.stop()
        if not engine_stopped_properly:
            raise RuntimeError("Can't stop engine properly")
        self._check_status_code(status, signal.SIGTERM)

    def restart(self):
        """
        Перезапуск движка
        """
        self.stop()
        self.start()

    def get_log(self):
        """
        Получить полный лог движка
        """
        self._assert_availability()
        self._read_new_logs()
        return self._engine_logs


    def assert_log(self, expect, message="Can't wait expected log", timeout=60):
        """
        Проверить наличие сообщение в логе движка
        :param expect: Список паттернов ожидаемых сообщений
        :param message: Ошибка, которая будет выброщена в случае отсуствии ожидаемых сообщений в логах
        :param timeout: Время ожидания появления сообщений в логе
        """
        start = time.time()
        expected_msgs = expect[:]

        while expected_msgs:
            self._assert_availability()
            self._read_new_logs()
            for index, log_line in enumerate(self._engine_logs):
                if not expected_msgs:
                    return

                remove_expected_index = None
                for i, expected_msg in enumerate(expected_msgs):
                    if re.search(expected_msg, log_line):
                        remove_expected_index = i
                        self._engine_logs[index] = ''
                        break

                if remove_expected_index is None:
                    self._assert_no_errors_in_log(log_line)
                else:
                    expected_msgs.pop(remove_expected_index)

            time.sleep(0.05)
            if time.time() - start > timeout:
                expected_str = json.dumps(obj=expected_msgs, indent=2)
                raise RuntimeError("{}; Missed messages: {}".format(message, expected_str))

    def get_stats(self, prefix="", timeout=60):
        """
        Получить последнюю стату движка
        :param prefix: Строковый префикс, который будет использоваться в качестве маски для статы
        :param timeout: Время ожидания очередной статы
        :return: Словарь со статой без префикса
        """
        self._stats_receiver.try_update_stats()
        self._stats_receiver.wait_next_stats(timeout)
        stats = self._stats_receiver.stats
        return {k[len(prefix):]: v for k, v in stats.items() if k.startswith(prefix)}

    def assert_stats(self, expected_added_stats, initial_stats=None,
                     prefix="", message="Can't wait expected stats", timeout=60):
        """
        Проверить стату движка
        :param expected_added_stats: Словарь с ожадаемой статой (можно подмножество)
        :param initial_stats: Словарь со статой, которая будет использоваться в качетсве начального значения
        :param prefix: Строковый префикс, который будет использоваться в качестве маски для статы
        :param message: Ошибка, которая будет выброщена в случае отсуствии ожидаемой статы
        :param timeout: Время ожидания появления статы
        """
        initial_stats = initial_stats or {}

        s = time.time()
        done = False
        got_added_stats = {}
        while not done:
            try:
                got_stats = self.get_stats(prefix, timeout - (time.time() - s))
                got_added_stats = {k: v - initial_stats.get(k, 0) for k, v in got_stats.items()}
            except:
                expected_str = json.dumps(
                    obj=expected_added_stats,
                    sort_keys=True,
                    indent=2,
                    default=lambda obj: str(obj)
                )
                got_str = json.dumps(
                    obj={k: got_added_stats.get(k) for k in expected_added_stats.keys()},
                    sort_keys=True,
                    indent=2,
                    default=lambda obj: str(obj)
                )
                raise RuntimeError("{}; Expected: {}\nGot: {}\n".format(message, expected_str, got_str))
            done = True
            for k, v in expected_added_stats.items():
                if k not in got_added_stats or v != got_added_stats[k]:
                    done = False
                    break

    def send_signal(self, signum):
        """
        Sends signal to engine
        :param signum: Signal number
        """
        self._engine_process.send_signal(signum)

    def wait_termination(self, timeout_sec):
        """
        Waits for engine termination via waitpid with given timeout
        :param timeout_sec: Maximum number of seconds to wait. RuntimeError is thrown in case of timeout exceeding
        :return: Tuple with: bool - is engine stopped properly
                             int  - status from waitpid
        """

        def raise_timeout(x, y):
            raise RuntimeError("Can't wait process")

        handler = signal.signal(signal.SIGALRM, raise_timeout)
        engine_stopped_properly = True
        try:
            signal.alarm(timeout_sec)
            _, status = os.waitpid(self._engine_process.pid, 0)
        except:
            engine_stopped_properly = False
            self.send_signal(signal.SIGKILL)
            _, status = os.waitpid(self._engine_process.pid, 0)

        signal.signal(signal.SIGALRM, handler)
        signal.alarm(0)

        return (engine_stopped_properly, status)

    def rpc_request(self, request, timeout=60):
        """
        Послать rpc запрос в движек
        :param request: Словарь с rpc запросом
        :return: Словарь с rpc ответом
        """
        return send_rpc_request(request, self._rpc_port, timeout)

    def _assert_availability(self):
        _, status = os.waitpid(self._engine_process.pid, os.WNOHANG)
        self._check_status_code(status)

    def _assert_no_errors(self):
        self._read_new_logs()
        for log_line in self._engine_logs:
            self._assert_no_errors_in_log(log_line)

    def _read_new_logs(self):
        new_logs = list(filter(None, self._log_file_read_fd.readlines()))
        if new_logs:
            print("\n=========== Got new logs ============")
            print("".join(new_logs).strip())
            print("=======================================")
            self._engine_logs += new_logs

    @staticmethod
    def _check_status_code(proc_status, expected_signal=None):
        if os.WCOREDUMP(proc_status):
            raise RuntimeError("Core dumped!")
        if not os.WIFEXITED(proc_status) and expected_signal != proc_status:
            raise RuntimeError('Engine exited with status: {}'.format(proc_status))

    @staticmethod
    def _assert_no_errors_in_log(log_line):
        for signal in ("SIGABRT", "SIGSEGV", "SIGBUS", "SIGFPE", "SIGILL", "SIGQUIT", "SIGSYS", "SIGXCPU", "SIGXFSZ"):
            if signal in log_line:
                raise RuntimeError("Got unexpected signal: {}".format(log_line))
        for issue in ("Warning", "Error", "Critical error", "Assertion", "dl_assert"):
            if re.search("\\b{}\\b".format(issue), log_line):
                raise RuntimeError("Got unexpected {}: {}".format(issue, log_line))
