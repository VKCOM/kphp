import re
import time
import pytest
from python.tests.curl.curl_test_case import CurlTestCase


class TestCurlResumableReuseHandle(CurlTestCase):
    test_case_uri="/test_curl_resumable_reuse_handle"

    def test_curl_resumable_reuse_handle(self):
        self.assertEqual(self._curl_request("/echo/test_get"), {
            "exec_result1": self._prepare_result("/echo/test_get", "GET"),
            "exec_result2": self._prepare_result("/echo/test_get", "GET")
        })

    def test_curl_resumable_reuse_handle_after_timeout(self):
        self.assertEqual(self._curl_request("/echo/test_get", timeout=0.1), {
            "exec_result1": False,
            "exec_result2": self._prepare_result("/echo/test_get", "GET")
        })

    @pytest.mark.kphp_skip
    def test_curl_resumable_timeout_sequence(self):
        url = "localhost:{}{}".format(self.web_server.http_port, "/echo/test_get")
        started_at = time.monotonic()
        response = self.web_server.http_post(
            uri="/test_curl_resumable_timeout_sequence",
            json={"url": url, "timeouts": [0.1, 0.1, 0.9]})
        elapsed = time.monotonic() - started_at

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json(), {
            "exec_results": [
                False,
                False,
                self._prepare_result("/echo/test_get", "GET")
            ]
        })
        # The upstream endpoint sleeps for 0.5 s. A timed-out transfer must release
        # the Easy before the next call starts; otherwise three transfers take about 1.5 s.
        self.assertLess(elapsed, 1.2)
