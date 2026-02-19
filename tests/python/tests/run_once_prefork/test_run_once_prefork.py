import os
import signal
import time

import pytest

from python.lib.testcase import WebServerAutoTestCase
from python.lib.kphp_builder import KphpBuilder
from python.lib.kphp_server import KphpServer


class TestRunOncePrefork(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options(options={"--http-port": None})

    def _start_run_once_server(self, runs_count, workers_num=1, auto_start=True):
        """Create a KPHP server configured for run-once prefork mode"""
        self.web_server.update_options(options={
                "--once={}".format(runs_count): True,
                "--workers-num": workers_num,
            })
        self.web_server.restart()

    def test_run_once_single(self):
        """Test basic run-once prefork mode: worker must execute script N times, then restart the process and repeat"""
        runs_count = 1
        self._start_run_once_server(runs_count=runs_count, workers_num=1)
        self.web_server.assert_stats(
            {
                "kphp_server.workers_general_requests_total_incoming_queries": self.cmpGe(2),
                "kphp_server.server_workers_started": self.cmpGe(2)
            },
            timeout=5
        )
        self.web_server.stop()

    def test_run_once_with_multiple_workers(self):
        """Test run-once with multiple workers"""
        runs_count = 10
        workers_num = 4
        self._start_run_once_server(runs_count=runs_count, workers_num=workers_num)

        self.web_server.assert_stats(
            {
                "kphp_server.workers_general_requests_total_incoming_queries": self.cmpGe(runs_count * workers_num),
                "kphp_server.server_workers_started": self.cmpGe(workers_num)
            },
            timeout=5
        )

        self.web_server.stop()

    def test_run_once_large_batch(self):
        """Test run-once with large number of runs (tests batching)"""
        runs_count = 1000
        self._start_run_once_server(runs_count=runs_count, workers_num=1)

        self.web_server.assert_stats(
            {
                "kphp_server.workers_general_requests_total_incoming_queries": self.cmpGe(runs_count),
                "kphp_server.server_workers_started": self.cmpGe(1)
            },
            timeout=5
        )

        self.web_server.stop()

    def test_run_once_infinite(self):
        """Test run-once with large number of runs (tests batching)"""
        runs_count = 2**31 - 1 # max int32
        self._start_run_once_server(runs_count=runs_count, workers_num=1)

        self.web_server.assert_stats(
            {
                "kphp_server.workers_general_requests_total_incoming_queries": self.cmpGe(2000),
                "kphp_server.server_workers_started": 1
            },
            timeout=5
        )

        self.web_server.stop()
