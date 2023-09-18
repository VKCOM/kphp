from python.lib.testcase import KphpServerAutoTestCase
from python.lib.stats_receiver import StatsReceiver, StatsType


class TestStatshouseSmoke(KphpServerAutoTestCase):
    WORKERS_NUM = 2

    @classmethod
    def extra_class_setup(cls):
        cls.statshouse = StatsReceiver("kphp_server", cls.kphp_server_working_dir, StatsType.STATSHOUSE, True)
        cls.statshouse.start()
        cls.kphp_server.update_options({
            "--workers-num": cls.WORKERS_NUM,
            "--statshouse-client": ":" + str(cls.statshouse.port),
        })

    @classmethod
    def extra_class_teardown(cls):
        cls.statshouse.stop()

    def _send_request(self):
        resp = self.kphp_server.http_get()
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "Hello world!")

    def test_statshouse_smoke(self):
        for _ in range(5):
            self._send_request()
            self.statshouse.wait_next_stats(timeout=5)
