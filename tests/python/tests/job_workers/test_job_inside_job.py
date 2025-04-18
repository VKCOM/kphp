import typing
import pytest

from python.lib.kphp_server import KphpServer
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestJobInsideJob(WebServerAutoTestCase):
    WORKERS_NUM = 12
    JOB_WORKERS_RATIO = 0.5
    JOB_WORKERS_NUM = WORKERS_NUM * JOB_WORKERS_RATIO

    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": cls.WORKERS_NUM,
            "--job-workers-ratio": cls.JOB_WORKERS_RATIO,
            "--verbosity-job-workers=2": True,
        })

    def _test_job_redirect_chain(self, *, data, redirect_chain_len, timeout_error=False):
        stats_before = self.web_server.get_stats()
        resp = self.web_server.http_post(
            uri="/test_simple_cpu_job",
            json={"data": data,
                  "tag": "redirect_to_job_worker",
                  "redirect-chain-len": redirect_chain_len,
                  })

        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [x ** 2 for x in cur_data], "stats": []} if not timeout_error else
                {"error": "Job client timeout", "error_code": -102}
                for cur_data in data
            ]})
        messages_cnt = len(data) * 2 * (redirect_chain_len + 1)
        if redirect_chain_len >= self.JOB_WORKERS_NUM:
            messages_cnt -= 1
        self.web_server.assert_stats(
            initial_stats=stats_before,
            timeout=10,
            expected_added_stats={
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_acquired": messages_cnt,
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_released": messages_cnt,
                "kphp_server.workers_job_memory_messages_shared_messages_buffer_acquire_fails": 0
            })
        return resp

    def test_simple_job_redirect(self):
        self._test_job_redirect_chain(data=[[1, 2, 3, 4], [7, 9, 12]], redirect_chain_len=1)

    def test_job_redirect_max_chain(self):
        self._test_job_redirect_chain(data=[[i for i in range(10)]], redirect_chain_len=self.JOB_WORKERS_NUM - 1)

    def test_job_redirect_too_long_chain(self):
        self._test_job_redirect_chain(data=[[i for i in range(10)]], redirect_chain_len=self.JOB_WORKERS_NUM,
                                      timeout_error=True)

    def test_job_worker_self_lock_freedom(self):
        data = [[i for i in range(10)]]
        resp = self.web_server.http_post(
            uri="/test_simple_cpu_job",
            json={"data": data,
                  "tag": "self_lock_job",
                  "master-port": typing.cast(KphpServer, self.web_server).master_port
                  })

        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [x ** 2 for x in cur_data], "stats": []} for cur_data in data
            ]})
