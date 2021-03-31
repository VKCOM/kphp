from multiprocessing.dummy import Pool as ThreadPool

from python.lib.testcase import KphpServerAutoTestCase


class TestComplexScenarioJob(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--job-workers-num": 3,
            "--workers-num": 5
        })

    def do_no_filter_scenario(self):
        resp = self.kphp_server.http_post(
            uri="/test_complex_scenario",
            json={"master-port": self.kphp_server.master_port}
        )
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "mc_stats": {"string_stats": 11, "float_stats": 4, "int_stats": 16},
            "mc_stats_full": {"string_stats": 22, "float_stats": 4, "int_stats": 16},
            "mc_stats_fast": {"string_stats": 17, "float_stats": 0, "int_stats": 14},
            "rpc_stats": {"string_stats": 15, "float_stats": 27, "int_stats": 145},
            "rpc_filtered_stats": {"string_stats": 0, "float_stats": 0, "int_stats": 0}
        })

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
        self.assertEqual(resp.json(), {
            "mc_stats": {"string_stats": 3, "float_stats": 1, "int_stats": 1},
            "mc_stats_full": {"string_stats": 4, "float_stats": 1, "int_stats": 1},
            "mc_stats_fast": {"string_stats": 3, "float_stats": 0, "int_stats": 1},
            "rpc_stats": {"string_stats": 3, "float_stats": 1, "int_stats": 12},
            "rpc_filtered_stats": {"string_stats": 3, "float_stats": 1, "int_stats": 12}
        })

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
