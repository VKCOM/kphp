import socket

from python.lib.testcase import KphpServerAutoTestCase
from python.lib.http_client import RawResponse

class TestGzipHeaderReset(KphpServerAutoTestCase):
    def test_single_gzip_buffer(self):
        response = self.gzip_request("gzip")
        self.assertEqual(response.headers["Content-Encoding"], "gzip")

    def test_single_gzip_buffer_closed(self):
        response = self.gzip_request("reset")
        with self.assertRaises(KeyError):
            _ = response.headers["Content-Encoding"]

    def test_ignore_second_gzip_handler(self):
        response = self.gzip_request("ignore-second-handler")
        with self.assertRaises(KeyError):
            _ = response.headers["Content-Encoding"]

    def gzip_request(self, type):
        url = "/test_script_gzip_header?type=" + type
        return self.kphp_server.http_get(url, headers={
            "Host": "localhost",
            "Accept-Encoding": "gzip"
        })
