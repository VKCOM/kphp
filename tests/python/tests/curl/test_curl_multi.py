import pytest
from python.tests.curl.curl_test_case import CurlMultiTestCase


class TestCurl(CurlMultiTestCase):
    test_case_uri="/test_curl_multi"

    def test_curl_multi_transfers(self):
        self.assertEqual(self._curl_multi_request("/echo/multi/payload_1", "/echo/multi/payload_2"), {
            "mutli_ret_code": 0,
            "easy_result1": "payload_1",
            "easy_result2": "payload_2"
        })
