import pytest

from python.lib.testcase import WebServerAutoTestCase


class TestGzipHeaderReset(WebServerAutoTestCase):
    def test_single_gzip_buffer(self):
        response = self.gzip_request("gzip")
        self.assertEqual(response.headers["Content-Encoding"], "gzip")

    def test_single_gzip_buffer_closed(self):
        response = self.gzip_request("reset")
        with self.assertRaises(KeyError):
            _ = response.headers["Content-Encoding"]

    def test_gzip_handler_after_reset(self):
        response = self.gzip_request("gzhandler-after-reset")
        self.assertEqual(response.headers["Content-Encoding"], "gzip")

    def test_deflate_handler_after_reset(self):
        response = self.deflate_request("gzhandler-after-reset")
        self.assertEqual(response.headers["Content-Encoding"], "deflate")

    def test_gzip_handler_with_nested_buffer(self):
        response = self.gzip_request("gzhandler-with-nested-buffer")
        self.assertEqual(response.headers["Content-Encoding"], "gzip")

    def test_gzip_without_handler(self):
        response = self.gzip_request("gzhandler-absent")
        with self.assertRaises(KeyError):
            _ = response.headers["Content-Encoding"]

    @pytest.mark.k2_skip
    def test_ignore_second_gzip_handler(self):
        response = self.gzip_request("ignore-second-handler")
        with self.assertRaises(KeyError):
            _ = response.headers["Content-Encoding"]

    def deflate_request(self, type):
        url = "/test_script_gzip_header?type=" + type
        return self.web_server.http_get(url, headers={
            "Host": "localhost",
            "Accept-Encoding": "deflate"
        })

    def gzip_request(self, type):
        url = "/test_script_gzip_header?type=" + type
        return self.web_server.http_get(url, headers={
            "Host": "localhost",
            "Accept-Encoding": "gzip"
        })
