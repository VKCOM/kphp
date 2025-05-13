import pytest

from python.lib.testcase import WebServerAutoTestCase


class TestGzipHeaderReset(WebServerAutoTestCase):
    def test_single_gzip_buffer(self):
        response = self.gzip_request("gzip")
        self.assertEqual(response.headers["Content-Encoding"], "gzip")

    @pytest.mark.k2_skip
    def test_single_gzip_buffer_closed(self):
        response = self.gzip_request("reset")
        with self.assertRaises(KeyError):
            _ = response.headers["Content-Encoding"]

    @pytest.mark.k2_skip
    def test_ignore_second_gzip_handler(self):
        response = self.gzip_request("ignore-second-handler")
        with self.assertRaises(KeyError):
            _ = response.headers["Content-Encoding"]

    def gzip_request(self, type):
        url = "/test_script_gzip_header?type=" + type
        return self.web_server.http_get(url, headers={
            "Host": "localhost",
            "Accept-Encoding": "gzip"
        })
