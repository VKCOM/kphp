from multiprocessing.dummy import Pool as ThreadPool
from python.lib.testcase import KphpServerAutoTestCase


class TestWebServerStats(KphpServerAutoTestCase):
    WORKERS_NUM = 2

    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": cls.WORKERS_NUM
        })

    def convert_webserver_stats(self, response):
        stats_dict = {}
        for stat_line in response.text.splitlines():
            key, value = stat_line.split(':', 1)
            stats_dict[key] = int(value)
        return stats_dict

    def test_total_workers_count(self):
        engine_stats = self.kphp_server.http_get("/get_webserver_stats")
        stats_dict = self.convert_webserver_stats(engine_stats)
        self.assertEqual(stats_dict["total_workers"], self.WORKERS_NUM)
