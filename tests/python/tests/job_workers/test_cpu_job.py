import pytest
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestCpuJob(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 4,
            "--job-workers-ratio": 0.5,
            "--verbosity-job-workers=2": True,
        })

    def test_simple_cpu_job(self):
        stats_before = self.web_server.get_stats(prefix="kphp_server.workers_job_memory_messages_")
        resp = self.web_server.http_post(
            uri="/test_simple_cpu_job",
            json={"data": [[1, 2, 3, 4], [7, 9, 12]]})

        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [1 * 1, 2 * 2, 3 * 3, 4 * 4], "stats": []},
                {"data": [7 * 7, 9 * 9, 12 * 12], "stats": []},
            ]})
        self.web_server.assert_stats(
            initial_stats=stats_before,
            prefix="kphp_server.workers_job_memory_messages_",
            expected_added_stats={
                "shared_messages_buffers_acquired": 4,
                "shared_messages_buffers_released": 4,
                "shared_messages_buffer_acquire_fails": 0,
            })

    def test_heavy_cpu_job(self):
        stats_before = self.web_server.get_stats(prefix="kphp_server.workers_job_memory_messages_")
        a = [1, 2, 3, 4, 5, 6]
        resp = self.web_server.http_post(
            uri="/test_simple_cpu_job",
            json={"data": [a * 10000, a * 1000, a * 100]})

        a2 = [x * x for x in a]
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": a2 * 10000, "stats": []},
                {"data": a2 * 1000, "stats": []},
                {"data": a2 * 100, "stats": []},
            ]})
        self.web_server.assert_stats(
            initial_stats=stats_before,
            prefix="kphp_server.workers_job_memory_messages_",
            expected_added_stats={
                "shared_messages_buffers_acquired": 6,
                "shared_messages_buffers_released": 6,
                "shared_messages_buffer_acquire_fails": 0,
                "extra_buffers_1mb_buffers_acquired": 2,
                "extra_buffers_1mb_buffers_released": 2,
                "extra_buffers_1mb_buffer_acquire_fails": 0
            })

    def test_simple_cpu_job_and_rpc_request_between(self):
        stats_before = self.web_server.get_stats()
        resp = self.web_server.http_post(
            uri="/test_cpu_job_and_rpc_usage_between",
            json={
                "data": [[1, 2, 3, 4], [7, 9, 12]],
                "master-port": self.web_server.master_port
            })

        self.assertEqual(resp.status_code, 200)
        result = resp.json()
        self.assertEqual(result["jobs-result"], [
            {"data": [1 * 1, 2 * 2, 3 * 3, 4 * 4], "stats": []},
            {"data": [7 * 7, 9 * 9, 12 * 12], "stats": []},
        ])
        self.assertEqual(result["stats"]["_"], "_")
        self.assertGreater(len(result["stats"]["result"]), 5)
        self.web_server.assert_stats(
            initial_stats=stats_before,
            expected_added_stats={
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_acquired": 4,
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_released": 4,
                "kphp_server.workers_job_memory_messages_shared_messages_buffer_acquire_fails": 0
            })

    def test_simple_cpu_job_and_mc_usage_between(self):
        stats_before = self.web_server.get_stats()
        resp = self.web_server.http_post(
            uri="/test_cpu_job_and_mc_usage_between",
            json={
                "data": [[1, 2, 3, 4], [7, 9, 12]],
                "master-port": self.web_server.master_port
            })

        self.assertEqual(resp.status_code, 200)
        result = resp.json()
        self.assertEqual(result["jobs-result"], [
            {"data": [1 * 1, 2 * 2, 3 * 3, 4 * 4], "stats": []},
            {"data": [7 * 7, 9 * 9, 12 * 12], "stats": []},
        ])
        self.assertTrue(result["stats"].startswith("cpu_usage"))
        self.web_server.assert_stats(
            initial_stats=stats_before,
            expected_added_stats={
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_acquired": 4,
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_released": 4,
                "kphp_server.workers_job_memory_messages_shared_messages_buffer_acquire_fails": 0
            })
