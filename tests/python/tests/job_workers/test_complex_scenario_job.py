import typing
import pytest

from multiprocessing.dummy import Pool as ThreadPool

from python.lib.kphp_server import KphpServer
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestComplexScenarioJob(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 18,
            "--job-workers-shared-memory-distribution-weights": '2,2,2,2,1,1,1,1,1,1',
            "--job-workers-ratio": 0.16,
            "--verbosity-job-workers=2": True,
        })

    def assert_stats_count(self, stats):
        self.assertGreater(stats["mc_stats"], 0)
        self.assertGreater(stats["mc_stats_full"], 0)
        self.assertGreater(stats["mc_stats_fast"], 0)
        self.assertGreater(stats["rpc_stats"], 0)
        self.assertGreaterEqual(stats["rpc_filtered_stats"], 0)

    def do_no_filter_scenario(self):
        resp = self.web_server.http_post(
            uri="/test_complex_scenario",
            json={"master-port": typing.cast(KphpServer, self.web_server).master_port}
        )
        self.assertEqual(resp.status_code, 200)
        self.assert_stats_count(resp.json())

    def do_filter_scenario(self):
        resp = self.web_server.http_post(
            uri="/test_complex_scenario",
            json={
                "master-port": typing.cast(KphpServer, self.web_server).master_port,
                "stat-names": [
                    "kphp_version", "hostname", "uptime", "RSS", "VM", "tot_idle_time",
                    "terminated_requests.memory_limit_exceeded",
                    "terminated_requests.timeout",
                    "terminated_requests.exception",
                    "terminated_requests.stack_overflow",
                    "terminated_requests.php_assert",
                    "terminated_requests.http_connection_close",
                    "terminated_requests.rpc_connection_close",
                    "terminated_requests.net_event_error",
                    "terminated_requests.post_data_loading_error",
                    "terminated_requests.unclassified",
                ]
            }
        )
        self.assertEqual(resp.status_code, 200)
        self.assert_stats_count(resp.json())

    def do_test(self, it):
        if it % 2 == 1:
            self.do_no_filter_scenario()
        else:
            self.do_filter_scenario()

    def test_complex_scenario_job(self):
        requests_count = 100
        with ThreadPool(5) as pool:
            for _ in pool.imap_unordered(self.do_test, range(requests_count)):
                pass
        self.web_server.assert_stats(
            timeout=10,
            prefix="kphp_server.workers_job_",
            expected_added_stats={
                "memory_messages_shared_messages_buffers_acquired": requests_count * 10,
                "memory_messages_shared_messages_buffers_released": requests_count * 10,
                "memory_messages_shared_messages_buffers_reserved": 164,
                "jobs_queue_size": 0,
                "jobs_sent": requests_count * 5,
                "jobs_replied": requests_count * 5,
                "pipe_errors_server_write": 0,
                "pipe_errors_server_read": 0,
                "pipe_errors_client_write": 0,
                "pipe_errors_client_read": 0,
            })
        self.assertKphpNoTerminatedRequests()
