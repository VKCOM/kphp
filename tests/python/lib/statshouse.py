import os
import subprocess
import signal
import time
from sys import platform

import psutil

from .port_generator import get_port


class Statshouse:
    def __init__(self, engine_name, working_dir):
        self._working_dir = working_dir
        self._port = get_port()
        self._statshouse_proc = None
        self._statshouse_file = os.path.join(working_dir, engine_name + ".statshouse")
        self._statshouse_file_write_fd = None
        self._statshouse_file_read_fd = None
        self._stats = {}

    @property
    def port(self):
        return self._port

    @property
    def stats(self):
        return self._stats

    def start(self):
        print("\nStarting statshouse server on port {}".format(self._port))
        self._statshouse_file_write_fd = open(self._statshouse_file, 'wb')
        self._statshouse_file_read_fd = open(self._statshouse_file, 'r')
        self._statshouse_proc = psutil.Popen(
            ["nc", "-lu", "" if platform == "darwin" else "-p", str(self._port)],
            stdout=self._statshouse_file_write_fd,
            stderr=subprocess.STDOUT,
            cwd=self._working_dir
        )
        if not self._statshouse_proc.is_running():
            self._statshouse_file_write_fd.close()
            possible_error = self._statshouse_file_read_fd.read() or "empty out"
            self._statshouse_file_read_fd.close()
            RuntimeError("Can't start statshouse server: " + possible_error)

    def stop(self):
        if self._statshouse_proc is None or not self._statshouse_proc.is_running():
            return

        print("\nStopping statshouse server on port {}".format(self._port))
        self._statshouse_proc.send_signal(signal.SIGKILL)
        os.waitpid(self._statshouse_proc.pid, 0)
        self._statshouse_file_read_fd.close()
        self._statshouse_file_write_fd.close()

    def wait_metrics(self, timeout=10):
        start = time.time()
        while not self.try_update_stats():
            if time.time() - start > timeout:
                raise RuntimeError("Waiting next statshouse metrics timeout")
            time.sleep(0.1)

    def try_update_stats(self):
        self._statshouse_file_read_fd.seek(0, os.SEEK_END)
        return bool(self._statshouse_file_read_fd.tell())
