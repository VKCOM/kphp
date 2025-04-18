import pytest
from python.lib.testcase import WebServerAutoTestCase

@pytest.mark.k2_skip_suite
class TestShutdownFunctionsExceptions(WebServerAutoTestCase):
    def test_exception_shutdown_function(self):
        # test that:
        # 1. we execute shutdown functions after the "uncaught exception" critical error
        # 2. that CurException is not visible inside shutdown functions
        resp = self.web_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_exception_warning"},
                {"op": "exception", "msg": "hello", "code": 123},
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.web_server.assert_log([
            "running shutdown handler",
            "Error 123: hello",
            "Unhandled ServerException",
        ], timeout=5)
