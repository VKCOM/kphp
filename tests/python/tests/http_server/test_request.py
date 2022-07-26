import urllib
import socket

from python.lib.testcase import KphpServerAutoTestCase
from python.lib.http_client import _RawResponse


class TestRequest(KphpServerAutoTestCase):
    def test_send_simple_request_path(self):
        response = self.kphp_server.http_request_raw([b"GET /status HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    def test_send_request_with_early_hints(self):
        msg = b"GET /?hints=yes HTTP/1.1\n\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(msg)
            early_hints = s.recv(len(msg))
            s.settimeout(1)
            response = b''
            try:
                response = s.recv(4096)
            except OSError:
                pass
            self.assertEqual(response, b'')
            s.settimeout(None)
            response = s.recv(4096)
        self.assertEqual(_RawResponse(early_hints).status_code, 103)
        self.assertEqual(_RawResponse(response).status_code, 200)
        self.assertEqual(_RawResponse(response).content, b"Hello world!")

    def test_send_request_with_query(self):
        response = self.kphp_server.http_request_raw([b"GET /status?foo=bar&baz=gaz HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

        response = self.kphp_server.http_request_raw([b"GET /status?%07%az=g%07z HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

        response = self.kphp_server.http_request_raw([
            b"".join([
                b"GET /status?",
                urllib.parse.urlencode({'русские': "символы"}).encode(),
                b" HTTP/1.1"])
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

        response = self.kphp_server.http_request_raw([
            b'GET /status?$&+,/:;=?@"\'<>%{}|^~[]`#$&+,/:;=?@"\'<>%{}|^~[]`# HTTP/1.1'
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    def test_send_request_with_fragment(self):
        response = self.kphp_server.http_request_raw([b"GET /status?#something HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    def test_send_request(self):
        response = self.kphp_server.http_request_raw([
            b"GET /status?foo=bar&baz=gaz#something HTTP/1.1"
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    def test_send_request_with_headers(self):
        response = self.kphp_server.http_request_raw([
            b"GET /status HTTP/1.1",
            b"foo: bar",
            b"baz: gaz"
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")
