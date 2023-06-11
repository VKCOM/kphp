import re
from python.tests.curl.curl_test_case import CurlTestCase


class TestCurlReuseHandle(CurlTestCase):
    test_case_uri="/test_curl_reuse_handle"

    def test_curl_reuse_handle(self):
        self.assertEqual(self._curl_request("/echo/test_get"), {
            "exec_result1": self._prepare_result("/echo/test_get", "GET"),
            "exec_result2": self._prepare_result("/echo/test_get", "GET")
        })

    def test_curl_reuse_handle_after_timeout(self):
        self.assertEqual(self._curl_request("/echo/test_get", timeout=0.1), {
            "exec_result1": False,
            "exec_result2": self._prepare_result("/echo/test_get", "GET")
        })
