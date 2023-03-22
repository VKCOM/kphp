from multiprocessing.dummy import Pool as ThreadPool
from python.lib.testcase import KphpServerAutoTestCase


class TestEngineStats(KphpServerAutoTestCase):
    WORKERS_NUM = 2

    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": cls.WORKERS_NUM
        })

    def convert_engine_stats(self, response):
        stats_dict = {}
        for stat_line in response.text.splitlines():
            key, value = stat_line.split(':', 1)
            stats_dict[key] = int(value)
        return stats_dict

    def test_total_workers_count(self):
        engine_stats = self.kphp_server.http_get("/get_engine_stats")
        stats_dict = self.convert_engine_stats(engine_stats)
        self.assertEqual(stats_dict["total_workers"], self.WORKERS_NUM)

    def do_request(self, it):
        engine_stats = self.kphp_server.http_get("/get_engine_stats")
        stats_dict = self.convert_engine_stats(engine_stats)
        return stats_dict

    def test_running_workers_count(self):
        requests_count = 5
        with ThreadPool(3) as pool:
            for stats in pool.imap_unordered(self.do_request, range(requests_count)):
                self.assertEqual(stats["total_workers"], stats["running_workers"] + stats["waiting_workers"] + stats["ready_for_accept_workers"])
