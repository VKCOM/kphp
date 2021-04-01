from multiprocessing.dummy import Pool as ThreadPool

from python.lib.testcase import KphpServerAutoTestCase


class TestComplexScenarioJob(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--job-workers-num": 3,
            "--workers-num": 5
        })

    def assert_stats_count(self, stats):
        self.assertGreater(stats["mc_stats"], 0)
        self.assertGreater(stats["mc_stats_full"], 0)
        self.assertGreater(stats["mc_stats_fast"], 0)
        self.assertGreater(stats["rpc_stats"], 0)
        self.assertGreaterEqual(stats["rpc_filtered_stats"], 0)

    def do_no_filter_scenario(self):
        resp = self.kphp_server.http_post(
            uri="/test_complex_scenario",
            json={"master-port": self.kphp_server.master_port}
        )
        self.assertEqual(resp.status_code, 200)
        self.assert_stats_count(resp.json())

    def do_filter_scenario(self):
        resp = self.kphp_server.http_post(
            uri="/test_complex_scenario",
            json={
                "master-port": self.kphp_server.master_port,
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
        requests_count = 500
        with ThreadPool(10) as pool:
            for _ in pool.imap_unordered(self.do_test, range(requests_count)):
                pass
        self.kphp_server.assert_stats(
            prefix="kphp_server.job_workers_",
            expected_added_stats={
                "job_queue_size": 0,
                "currently_memory_slices_used": 0,
                "max_shared_memory_slices_count": 1024,
                "jobs_sent": requests_count * 5,
                "jobs_replied": requests_count * 5,
                "errors_pipe_server_write": 0,
                "errors_pipe_server_read": 0,
                "errors_pipe_client_write": 0,
                "errors_pipe_client_read": 0,
            })
        self.assertKphpNoTerminatedRequests()
