from python.lib.testcase import KphpServerAutoTestCase
from python.lib.stats_receiver import StatsReceiver, StatsType
import time

class TestServerConfig(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.statshouse = StatsReceiver("kphp_server", cls.kphp_server_working_dir, StatsType.STATSHOUSE)
        cls.statshouse.start()
        cls.kphp_server.update_options({
            "--server-config": "data/server-config.yml",
            "--statshouse-client": "localhost:" + str(cls.statshouse.port),
        })

    @classmethod
    def extra_class_teardown(cls):
        cls.statshouse.stop()

    def _assert_statshouse_stats(self, needle, offset=0, timeout=5):
        start = time.time()
        while self.statshouse.stats.find(needle, offset) == -1:
            if time.time() - start > timeout:
                raise RuntimeError("Can't find string {} in StatsHouse stats with offset={}".format(needle, offset))
            self.statshouse.try_update_stats()
            time.sleep(0.05)


    def test_names_from_config(self):
        resp = self.kphp_server.http_request("/server-status", http_port=self.kphp_server.master_port)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.content.decode("utf-8").split('\n')[0].split("\t")[1], "custom_cluster_name")
        self.assertEqual(resp.content.decode("utf-8").split('\n')[1].split("\t")[1], "custom_server_name")
        self._assert_statshouse_stats("custom_cluster_name")
        self._assert_statshouse_stats("custom_server_name")
