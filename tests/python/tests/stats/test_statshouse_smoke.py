import time
import pytest

from python.lib.testcase import WebServerAutoTestCase
from python.lib.stats_receiver import StatsReceiver, StatsType


@pytest.mark.k2_skip_suite
class TestStatshouseSmoke(WebServerAutoTestCase):
    WORKERS_NUM = 2

    @classmethod
    def extra_class_setup(cls):
        cls.statshouse = StatsReceiver("kphp_server", cls.web_server_working_dir, StatsType.STATSHOUSE)
        cls.statshouse.start()
        cls.web_server.update_options({
            "--workers-num": cls.WORKERS_NUM,
            "--statshouse-client": "localhost:" + str(cls.statshouse.port),
        })

    @classmethod
    def extra_class_teardown(cls):
        cls.statshouse.stop()

    def _send_request(self):
        resp = self.web_server.http_get()
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "Hello world!")

    def _assert_statshouse_stats(self, needle, offset=0, timeout=5):
        start = time.time()
        while self.statshouse.stats.find(needle, offset) == -1:
            if time.time() - start > timeout:
                raise RuntimeError("Can't find string {} in StatsHouse stats with offset={}".format(needle, offset))
            self.statshouse.try_update_stats()
            time.sleep(0.05)

    def test_statshouse_smoke(self):
        self._assert_statshouse_stats("kphp_server_workers")
        for _ in range(5):
            offset = len(self.statshouse.stats)
            self._send_request()
            self._assert_statshouse_stats("kphp_request_time", offset)
