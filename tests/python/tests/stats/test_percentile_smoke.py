import pytest
from multiprocessing.dummy import Pool as ThreadPool

from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestPercentileSmoke(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 210
        })

    def _send_request(self, ignore_id):
        resp = self.web_server.http_get()
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "Hello world!")

    def test_percentile_smoke(self):
        initial_stats = self.web_server.get_stats(prefix="kphp_server.")
        with ThreadPool(20) as p:
            p.map(self._send_request, range(400))

        self.web_server.assert_stats(
            initial_stats=initial_stats,
            prefix="kphp_server.",
            expected_added_stats={
                "workers_general_memory_script_usage_p50": self.cmpGt(0),
                "workers_general_memory_script_usage_p95": self.cmpGt(0),
                "workers_general_memory_script_usage_p99": self.cmpGt(0),
                "workers_general_memory_script_real_usage_p50": self.cmpGt(0),
                "workers_general_memory_script_real_usage_p95": self.cmpGt(0),
                "workers_general_memory_script_real_usage_p99": self.cmpGt(0),
                "workers_general_requests_script_time_p50": self.cmpGe(0.01),
                "workers_general_requests_script_time_p95": self.cmpGe(0.01),
                "workers_general_requests_script_time_p99": self.cmpGe(0.01),
                "workers_general_requests_net_time_p50": self.cmpGe(0.01),
                "workers_general_requests_net_time_p95": self.cmpGe(0.01),
                "workers_general_requests_net_time_p99": self.cmpGe(0.01),
                "workers_general_requests_working_time_p50": self.cmpGe(0.02),
                "workers_general_requests_working_time_p95": self.cmpGe(0.02),
                "workers_general_requests_working_time_p99": self.cmpGe(0.02)
            }
        )
