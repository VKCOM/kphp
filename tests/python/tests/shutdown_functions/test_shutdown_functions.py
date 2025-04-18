import pytest
from python.lib.testcase import WebServerAutoTestCase

@pytest.mark.k2_skip_suite
class TestShutdownFunctions(WebServerAutoTestCase):
    def test_fork_wait(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_fork_wait"},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)
        self.web_server.assert_log([
            "before fork",
            "after fork",
            "before yield",
            "after yield",
            "after wait",
        ], timeout=5)
