import pytest
from python.tests.curl.curl_test_case import CurlTestCase


class TestCurlReset(CurlTestCase):
    test_case_uri="/test_curl_reset_handle"

    def test_curl_reset_handle(self):
        self.assertEqual(self._curl_request("/echo/reset_handle"), {
            "exec_result1": self._prepare_result("/echo/reset_handle/before", "GET"),
            "exec_result2": False,
            "exec_result3": self._prepare_result("/echo/reset_handle/after", "GET")
        })
