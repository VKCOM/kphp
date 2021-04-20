from python.lib.testcase import KphpServerAutoTestCase


class TestJobErrors(KphpServerAutoTestCase):
    JOB_MEMORY_LIMIT_ERROR = -101
    JOB_TIMEOUT_ERROR = -102
    JOB_EXCEPTION_ERROR = -103
    JOB_STACK_OVERFLOW_ERROR = -104
    JOB_PHP_ASSERT_ERROR = -105
    JOB_NO_REPLY = -2001

    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--job-workers-num": 2
        })

    def _assert_result(self, stats_before, resp, error_code, buffers=4):
        self.assertEqual(resp.status_code, 200)

        job_result = resp.json()["jobs-result"]
        self.assertEqual(job_result[0]["error_code"], error_code)
        self.assertEqual(job_result[1]["error_code"], error_code)
        self.kphp_server.assert_stats(
            initial_stats=stats_before,
            expected_added_stats={
                "kphp_server.job_workers_memory_messages_buffers_acquired": buffers,
                "kphp_server.job_workers_memory_messages_buffers_released": buffers,
                "kphp_server.job_workers_memory_messages_buffer_acquire_fails": 0
            })

    def job_error_test_impl(self, error_type, error_code, buffers=4):
        stats_before = self.kphp_server.get_stats()
        resp = self.kphp_server.http_post(
            uri="/test_job_errors",
            json={
                "tag": "x2_with_error",
                "error-type": error_type,
                "data": [[1, 2, 3, 4], [7, 9, 12]]
            })
        self._assert_result(stats_before, resp, error_code, buffers)

    def test_job_script_timeout_error(self):
        stats_before = self.kphp_server.get_stats()
        script_timeout_sec = 1
        resp = self.kphp_server.http_post(
            uri="/test_job_script_timeout_error",
            json={
                "tag": "x2_with_sleep",
                "script-timeout": script_timeout_sec,
                "job-sleep-time-sec": script_timeout_sec * 5,
                "data": [[1, 2, 3, 4], [7, 9, 12]]
            })
        self._assert_result(stats_before, resp, self.JOB_TIMEOUT_ERROR)
        self.kphp_server.assert_log(2 * ["Critical error during script execution: timeout exit"])

    def test_job_memory_limit_error(self):
        self.job_error_test_impl("memory_limit", self.JOB_MEMORY_LIMIT_ERROR)
        self.kphp_server.assert_log(2 * [
            "Critical error during script execution: memory limit exit",
            "Warning: Can't allocate \\d+ bytes"
        ])

    def test_job_exception_error(self):
        self.job_error_test_impl("exception", self.JOB_EXCEPTION_ERROR)
        self.kphp_server.assert_log(2 * [
            "Critical error during script execution",
            "Error 0: Test exception"
        ])

    def test_job_stack_overflow_error(self):
        self.job_error_test_impl("stack_overflow", self.JOB_STACK_OVERFLOW_ERROR)
        self.kphp_server.assert_log(2 * [
            "Critical error during script execution: sigsegv\\(stack overflow\\)",
            "Error -1: Callstack overflow"
        ])

    def test_job_php_assert_error(self):
        self.job_error_test_impl("php_assert", self.JOB_PHP_ASSERT_ERROR)
        self.kphp_server.assert_log(2 * [
            'Warning: Critical error "Test php_assert" in file',
            "Critical error during script execution: php assert error"
        ])

    def test_job_sigsegv(self):
        self.job_error_test_impl("sigsegv", self.JOB_TIMEOUT_ERROR, 2)
        self.kphp_server.assert_log(2 * [
            "Error -2: Segmentation fault"
        ])

    def test_job_client_oom(self):
        stats_messages_before = self.kphp_server.get_stats("kphp_server.job_workers_memory_messages_")
        stats_extra_buffers_before = self.kphp_server.get_stats("kphp_server.job_workers_memory_extra_buffers_")

        resp = self.kphp_server.http_get("/test_client_too_big_request")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {"job-id": False})

        self.kphp_server.assert_log(["Warning: Can't send job: too big request"])
        self.kphp_server.assert_stats(
            prefix="kphp_server.job_workers_memory_messages_",
            initial_stats=stats_messages_before,
            expected_added_stats={"buffers_acquired": 1, "buffers_released": 1, "buffer_acquire_fails": 0})
        self.kphp_server.assert_stats(
            prefix="kphp_server.job_workers_memory_extra_buffers_",
            initial_stats=stats_extra_buffers_before,
            expected_added_stats={
                "1mb_buffer_acquire_fails": 6, "1mb_buffers_acquired": 4, "1mb_buffers_released": 4,
                "2mb_buffer_acquire_fails": 4, "2mb_buffers_acquired": 2, "2mb_buffers_released": 2,
                "4mb_buffer_acquire_fails": 3, "4mb_buffers_acquired": 1, "4mb_buffers_released": 1,
                "8mb_buffer_acquire_fails": 1, "8mb_buffers_acquired": 2, "8mb_buffers_released": 2,
                "16mb_buffer_acquire_fails": 1, "16mb_buffers_acquired": 0, "16mb_buffers_released": 0,
                "32mb_buffer_acquire_fails": 1, "32mb_buffers_acquired": 0, "32mb_buffers_released": 0,
                "64mb_buffer_acquire_fails": 1, "64mb_buffers_acquired": 0, "64mb_buffers_released": 0,
            })

    def test_big_response(self):
        stats_extra_buffers_before = self.kphp_server.get_stats("kphp_server.job_workers_memory_extra_buffers_")
        self.job_error_test_impl("big_response", self.JOB_NO_REPLY, 6)
        self.kphp_server.assert_log(2 * [
            "Warning: Can't store job response: too big response"
        ])
        self.kphp_server.assert_stats(
            prefix="kphp_server.job_workers_memory_extra_buffers_",
            initial_stats=stats_extra_buffers_before,
            expected_added_stats={
                "1mb_buffer_acquire_fails": self.cmpGe(6), "1mb_buffers_acquired": self.cmpGe(4),
                "1mb_buffers_released": self.cmpGe(4), "2mb_buffers_released": self.cmpGe(2),
                "2mb_buffer_acquire_fails": self.cmpGe(4), "2mb_buffers_acquired": self.cmpGe(2),
                "4mb_buffer_acquire_fails": self.cmpGe(3), "4mb_buffers_acquired": self.cmpGe(1),
                "4mb_buffers_released": self.cmpGe(1), "8mb_buffers_released": self.cmpGe(2),
                "8mb_buffer_acquire_fails": self.cmpGe(1), "8mb_buffers_acquired": self.cmpGe(2),
                "16mb_buffer_acquire_fails": self.cmpGe(1), "16mb_buffers_acquired": self.cmpGe(0),
                "16mb_buffers_released": self.cmpGe(0), "32mb_buffers_released": self.cmpGe(0),
                "32mb_buffer_acquire_fails": self.cmpGe(1), "32mb_buffers_acquired": self.cmpGe(0),
                "64mb_buffer_acquire_fails": self.cmpGe(1), "64mb_buffers_acquired": self.cmpGe(0),
                "64mb_buffers_released": self.cmpGe(0),
            })
