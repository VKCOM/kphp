import pytest
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestJobNoReply(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 4,
            "--job-workers-ratio": 0.5,
            "--verbosity-job-workers=2": True,
        })

    def _test_send_job_no_reply_impl(self, *, data, send_timeout, job_sleep_time):
        stats_before = self.web_server.get_stats()

        resp = self.web_server.http_post(
            uri="/test_send_job_no_reply",
            json={
                "data": data,
                "tag": "x2_no_reply",
                "job-sleep-time-sec": job_sleep_time,
                "send-timeout": send_timeout,
            })
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), "Success")
        if job_sleep_time < send_timeout:
            self.web_server.assert_log(["Finish no reply job: sum = {}".format(sum(arr)) for arr in data],
                                        "Some no-reply jobs weren't completed", timeout=10)
        self.web_server.assert_stats(
            initial_stats=stats_before,
            timeout=10,
            expected_added_stats={
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_acquired": len(data),
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_released": len(data),
                "kphp_server.workers_job_memory_messages_shared_messages_buffer_acquire_fails": 0
            })

    def test_job_no_reply_basic(self):
        self._test_send_job_no_reply_impl(data=[[1, 2, 3, 4], [7, 9, 12]], send_timeout=2, job_sleep_time=1)

    def test_job_no_reply_timeout_exceeded(self):
        self._test_send_job_no_reply_impl(data=[[101, 102, 103], [201, 202, 203]], send_timeout=1, job_sleep_time=2)
        self.web_server.assert_log(2 * ["Critical error during script execution: timeout exit"])
