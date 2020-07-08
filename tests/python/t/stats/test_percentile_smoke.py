from multiprocessing.dummy import Pool as ThreadPool

from python.lib.testcase import KphpServerAutoTestCase


class TestPercentileSmoke(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 210
        })

    def _send_request(self, ignore_id):
        resp = self.kphp_server.http_get()
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "Hello world!")

    def test_percentile_smoke(self):
        initial_stats = self.kphp_server.get_stats(prefix="kphp_server.")
        with ThreadPool(20) as p:
            p.map(self._send_request, range(400))

        self.kphp_server.assert_stats(
            initial_stats=initial_stats,
            prefix="kphp_server.",
            expected_added_stats={
                "memory_script_usage_percentile_50": self.cmpGt(0),
                "memory_script_usage_percentile_95": self.cmpGt(0),
                "memory_script_usage_percentile_99": self.cmpGt(0),
                "memory_script_real_usage_percentile_50": self.cmpGt(0),
                "memory_script_real_usage_percentile_95": self.cmpGt(0),
                "memory_script_real_usage_percentile_99": self.cmpGt(0),
                "requests_script_time_percentile_50": self.cmpGe(0.01),
                "requests_script_time_percentile_95": self.cmpGe(0.01),
                "requests_script_time_percentile_99": self.cmpGe(0.01),
                "requests_net_time_percentile_50": self.cmpGe(0.01),
                "requests_net_time_percentile_95": self.cmpGe(0.01),
                "requests_net_time_percentile_99": self.cmpGe(0.01),
                "requests_working_time_percentile_50": self.cmpGe(0.02),
                "requests_working_time_percentile_95": self.cmpGe(0.02),
                "requests_working_time_percentile_99": self.cmpGe(0.02)
            }
        )
