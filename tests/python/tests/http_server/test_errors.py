import pytest
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestErrors(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 1
        })

    def test_script_errors(self):
        stats_before = self.web_server.get_stats()

        for i in range(5):
            response = self.web_server.http_request(uri="/test_script_errors", method='GET')
            self.assertEqual(500, response.status_code)
            self.assertTrue(len(response.content) > 0)
            self.web_server.assert_log([
                'Warning: Critical error "Test error" in file',
                "Critical error during script execution: php assert error"
            ])

            response = self.web_server.http_get("/pid")
            self.assertEqual(200, response.status_code)

        self.web_server.assert_stats(
            timeout=5,
            initial_stats=stats_before,
            expected_added_stats={
                # Successful requests must not inherit the preceding script error.
                "kphp_server.workers_general_errors_php_assert": 5,
                # ensure shared data is not leaked
                "kphp_server.server_workers_strange_dead": 0,
                "kphp_server.server_workers_killed": 0,
                "kphp_server.server_workers_failed": 0,
                "kphp_server.server_workers_dead": 0,
                "kphp_server.server_workers_terminated": 0,
            })
