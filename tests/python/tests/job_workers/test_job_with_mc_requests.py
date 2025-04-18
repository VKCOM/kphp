import pytest
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestJobWithRpcRequests(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 4,
            "--job-workers-ratio": 0.5,
            "--verbosity-job-workers=2": True,
        })

    def test_job_with_mc_query(self):
        resp = self.web_server.http_post(
            uri="/test_simple_cpu_job",
            json={
                "tag": "x2_with_mc_request",
                "master-port": self.web_server.master_port,
                "data": [[1, 2, 3, 4], [7, 9, 12]]
            })

        self.assertEqual(resp.status_code, 200)
        job_result = resp.json()["jobs-result"]

        self.assertEqual(job_result[0]["data"], [1 * 1, 2 * 2, 3 * 3, 4 * 4])
        self.assertTrue(job_result[0]["stats"][0].startswith("cpu_usage"))

        self.assertEqual(job_result[1]["data"], [7 * 7, 9 * 9, 12 * 12])
        self.assertTrue(job_result[1]["stats"][0].startswith("cpu_usage"))

    def test_job_with_mc_query_and_rpc_request_between(self):
        resp = self.web_server.http_post(
            uri="/test_cpu_job_and_rpc_usage_between",
            json={
                "tag": "x2_with_mc_request",
                "data": [[1, 2, 3, 4], [7, 9, 12]],
                "master-port": self.web_server.master_port
            })

        self.assertEqual(resp.status_code, 200)
        result = resp.json()

        job_result = result["jobs-result"]
        self.assertEqual(job_result[0]["data"], [1 * 1, 2 * 2, 3 * 3, 4 * 4])
        self.assertTrue(job_result[0]["stats"][0].startswith("cpu_usage"))

        self.assertEqual(job_result[1]["data"], [7 * 7, 9 * 9, 12 * 12])
        self.assertTrue(job_result[1]["stats"][0].startswith("cpu_usage"))

        self.assertEqual(result["stats"]["_"], "_")
        self.assertGreater(len(result["stats"]["result"]), 5)

    def test_job_with_mc_query_and_mc_request_between(self):
        resp = self.web_server.http_post(
            uri="/test_cpu_job_and_mc_usage_between",
            json={
                "tag": "x2_with_mc_request",
                "data": [[1, 2, 3, 4], [7, 9, 12]],
                "master-port": self.web_server.master_port
            })

        self.assertEqual(resp.status_code, 200)
        result = resp.json()

        job_result = result["jobs-result"]
        self.assertEqual(job_result[0]["data"], [1 * 1, 2 * 2, 3 * 3, 4 * 4])
        self.assertTrue(job_result[0]["stats"][0].startswith("cpu_usage"))

        self.assertEqual(job_result[1]["data"], [7 * 7, 9 * 9, 12 * 12])
        self.assertTrue(job_result[1]["stats"][0].startswith("cpu_usage"))

        self.assertTrue(result["stats"].startswith("cpu_usage"))
