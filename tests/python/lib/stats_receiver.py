import os
import subprocess
import signal
import time
import re
from sys import platform

import psutil

from .port_generator import get_port


class StatsReceiver:
    def __init__(self, engine_name, working_dir):
        self._working_dir = working_dir
        self._port = get_port()
        self._stats_proc = None
        self._stats_file = os.path.join(working_dir, engine_name + ".stats")
        self._stats_file_write_fd = None
        self._stats_file_read_fd = None
        self._stats = {}

    @property
    def port(self):
        return self._port

    @property
    def stats(self):
        return self._stats

    def start(self):
        print("\nStarting stats receiver on port {}".format(self._port))
        self._stats_file_write_fd = open(self._stats_file, 'wb')
        self._stats_file_read_fd = open(self._stats_file, 'r')
        self._stats_proc = psutil.Popen(
            ["nc", "-l", "" if platform == "darwin" else "-p", str(self._port)],
            stdout=self._stats_file_write_fd,
            stderr=subprocess.STDOUT,
            cwd=self._working_dir
        )
        if not self._stats_proc.is_running():
            self._stats_file_write_fd.close()
            possible_error = self._stats_file_read_fd.read() or "empty out"
            self._stats_file_read_fd.close()
            RuntimeError("Can't start stats receiver: " + possible_error)

    def stop(self):
        if self._stats_proc is None or not self._stats_proc.is_running():
            return

        print("\nStopping stats receiver on port {}".format(self._port))
        self._stats_proc.send_signal(signal.SIGKILL)
        os.waitpid(self._stats_proc.pid, 0)
        self._stats_file_read_fd.close()
        self._stats_file_write_fd.close()

    def wait_next_stats(self, timeout=60):
        start = time.time()
        while not self.try_update_stats():
            if time.time() - start > timeout:
                raise RuntimeError("Waiting next stats timeout")
            time.sleep(0.05)

    def try_update_stats(self):
        new_stats = {}
        lines = self._stats_file_read_fd.readlines()
        for stat_line in filter(None, lines):
            if stat_line[-1] != "\n":
                print(lines)
                raise RuntimeError("Got bad stat line: {}".format(stat_line))
            stat, value = stat_line.split(":")
            value, _ = value.split("|")
            value = float(value.strip())
            new_stats[stat.strip()] = value.is_integer() and int(value) or value

        if not new_stats:
            return False
        if self._stats and len(self._stats) > len(new_stats):
            raise RuntimeError("Got inconsistent stats count: old={} new={}".format(len(self._stats), len(new_stats)))
        # HACK: replace prefix for kphp server stats
        self._stats = {re.sub("^kphp_stats\\..+\\.", "kphp_server.", k): v for k, v in new_stats.items()}
        return True
