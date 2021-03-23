from python.lib.testcase import KphpServerAutoTestCase


class TestJobErrors(KphpServerAutoTestCase):
    JOB_MEMORY_LIMIT_ERROR = -101
    JOB_TIMEOUT_ERROR = -102
    JOB_EXCEPTION_ERROR = -103
    JOB_STACK_OVERFLOW_ERROR = -104
    JOB_PHP_ASSERT_ERROR = -105

    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--job-workers-num": 2
        })
        cls.kphp_server.ignore_log_errors()

    def test_job_script_timeout_error(self):
        script_timeout_sec = 1
        resp = self.kphp_server.http_post(
            uri="/test_job_script_timeout_error",
            json={
                "tag": "x2_with_sleep",
                "script-timeout": script_timeout_sec,
                "job-sleep-time-sec": script_timeout_sec * 5,
                "data": [[1, 2, 3, 4], [7, 9, 12]]
            })

        self.assertEqual(resp.status_code, 200)

        job_result = resp.json()["jobs-result"]
        self.assertEqual(job_result[0]["error_code"], self.JOB_TIMEOUT_ERROR)
        self.assertEqual(job_result[1]["error_code"], self.JOB_TIMEOUT_ERROR)

    def job_error_test_impl(self, error_type, error_code):
        resp = self.kphp_server.http_post(
            uri="/test_job_errors",
            json={
                "tag": "x2_with_error",
                "error-type": error_type,
                "data": [[1, 2, 3, 4], [7, 9, 12]]
            })

        self.assertEqual(resp.status_code, 200)

        job_result = resp.json()["jobs-result"]
        self.assertEqual(job_result[0]["error_code"], error_code)
        self.assertEqual(job_result[1]["error_code"], error_code)

    def test_job_memory_limit_error(self):
        self.job_error_test_impl("memory_limit", self.JOB_MEMORY_LIMIT_ERROR)

    def test_job_exception_error(self):
        self.job_error_test_impl("exception", self.JOB_EXCEPTION_ERROR)

    def test_job_stack_overflow_error(self):
        self.job_error_test_impl("stack_overflow", self.JOB_STACK_OVERFLOW_ERROR)

    def test_job_php_assert_error(self):
        self.job_error_test_impl("php_assert", self.JOB_PHP_ASSERT_ERROR)
