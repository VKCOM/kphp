import pytest
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestShutdownFunctionsErrors(WebServerAutoTestCase):
    def test_critical_error(self):
        # with self.assertRaises(RuntimeError):
        resp = self.web_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_critical_error"},
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.web_server.assert_log([
            "running shutdown handler critical errors",
            "critical error from shutdown function",
        ], timeout=5)
