import socket

import pytest

from python.lib.http_client import RawResponse
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestHeadersSent(WebServerAutoTestCase):
    def test_on_flush(self):
        request = b"GET /test_headers_sent?type=flush HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.web_server.http_port))
            s.send(request)

            body = RawResponse(s.recv(4096)).content + s.recv(20)
            self.assertEqual(b"01", body)

    def test_header_sent_is_false_on_fastcgi_finish_request_when_no_flush(self):
        self.web_server.http_request(uri="/test_headers_sent?type=shutdown&flush=0")
        self.web_server.assert_log(["headers_sent\\(\\) after shutdown callback returns false"], timeout=10)

    def test_header_sent_is_true_on_fastcgi_finish_request_when_flush(self):
        request = b"GET /test_headers_sent?type=shutdown&flush=1 HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.web_server.http_port))
            s.send(request)

        self.web_server.assert_log(["headers_sent\\(\\) after shutdown callback returns true"], timeout=10)
