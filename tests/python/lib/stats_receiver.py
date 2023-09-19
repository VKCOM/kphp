import os
import subprocess
import signal
import time
import re
from sys import platform
from enum import Enum

import psutil

from .port_generator import get_port


class StatsType(Enum):
    STATSD = 0
    STATSHOUSE = 1


class StatsReceiver:
    def __init__(self, engine_name, working_dir, stats_type):
        self._working_dir = working_dir
        self._port = get_port()
        self._stats_proc = None
        self._stats_type = stats_type
        self._stats_file = os.path.join(working_dir, engine_name + "." + stats_type.name.lower())
        self._stats_file_write_fd = None
        self._stats_file_read_fd = None
        self._stats = {} if stats_type == StatsType.STATSD else ""

    @property
    def port(self):
        return self._port

    @property
    def stats(self):
        return self._stats

    def start(self):
        print("\nStarting stats receiver on port {}".format(self._port))
        self._stats_file_write_fd = open(self._stats_file, 'wb')
        self._stats_file_read_fd = open(self._stats_file, 'r',
                                        errors="replace" if self._stats_type == StatsType.STATSHOUSE else "strict")
        self._stats_proc = psutil.Popen(
            ["nc", "-l{}".format("u" if self._stats_type == StatsType.STATSHOUSE else ""),
                "" if platform == "darwin" else "-p", str(self._port)],
            stdout=self._stats_file_write_fd,
            stderr=subprocess.STDOUT,
            cwd=self._working_dir
        )
        if not self._stats_proc.is_running():
            self._stats_file_write_fd.close()
            possible_error = self._stats_file_read_fd.read() or "empty out"
            self._stats_file_read_fd.close()
            raise RuntimeError("Can't start stats receiver: " + possible_error)

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
        if self._stats_type == StatsType.STATSD:
            return self._try_update_stats_statsd()
        elif self._stats_type == StatsType.STATSHOUSE:
            return self._try_update_stats_statshouse()

    def _try_update_stats_statsd(self):
        new_stats = {}
        lines = self._stats_file_read_fd.readlines()
        for stat_line in filter(None, lines):
            if stat_line[-1] != "\n":
                return False
            try:
                stat, value = stat_line.split(":")
                value, _ = value.split("|")
            except ValueError:
                print("Got bad stat line: {}".format(stat_line))
                return False
            value = float(value.strip())
            new_stats[stat.strip()] = value.is_integer() and int(value) or value

        if not new_stats:
            return False
        # HACK: replace prefix for kphp server stats
        self._stats = {re.sub("^kphp_stats\\..+\\.", "kphp_server.", k): v for k, v in new_stats.items()}
        return True

    def _try_update_stats_statshouse(self):
        added_stats = self._stats_file_read_fd.read()
        self._stats += added_stats
        return len(added_stats) > 0
