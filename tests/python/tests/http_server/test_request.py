import urllib
import socket
import pytest

from python.lib.testcase import WebServerAutoTestCase
from python.lib.http_client import RawResponse


class TestRequest(WebServerAutoTestCase):
    def test_send_simple_request_path(self):
        response = self.web_server.http_request_raw([b"GET /status HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    @pytest.mark.k2_skip
    def test_send_request_with_early_hints(self):
        def print_log(header, body):
            print(header)
            print("=============================")
            print(*body.splitlines(True), sep="\n")
            print("=============================")

        msg = b"GET /?hints=yes HTTP/1.1\n\n"
        print_log("\nSending Raw HTTP request", msg)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.web_server.http_port))
            s.send(msg)
            s.settimeout(0.1)
            early_hints = RawResponse(s.recv(118))
            s.settimeout(1)
            pause = False
            try:
                s.recv(4096)
            except OSError:
                pause = True
            self.assertTrue(pause)
            s.settimeout(None)
            response = RawResponse(s.recv(4096))
        print_log("\nGot Raw HTTP 103 hints", early_hints.raw_bytes)
        print_log("\nGot Raw HTTP response", response.raw_bytes)
        self.assertEqual(early_hints.status_code, 103)
        self.assertEqual(early_hints.headers["Content-Type"], "text/plain or application/json")
        self.assertEqual(early_hints.headers["Link"], "</script.js>; rel=preload; as=script")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    @pytest.mark.k2_skip
    def test_send_request_with_query(self):
        response = self.web_server.http_request_raw([b"GET /status?foo=bar&baz=gaz HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

        response = self.web_server.http_request_raw([b"GET /status?%07%az=g%07z HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

        response = self.web_server.http_request_raw([
            b"".join([
                b"GET /status?",
                urllib.parse.urlencode({'русские': "символы"}).encode(),
                b" HTTP/1.1"])
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

        response = self.web_server.http_request_raw([
            b'GET /status?$&+,/:;=?@"\'<>%{}|^~[]`#$&+,/:;=?@"\'<>%{}|^~[]`# HTTP/1.1'
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    def test_send_request_with_fragment(self):
        response = self.web_server.http_request_raw([b"GET /status?#something HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    def test_send_request(self):
        response = self.web_server.http_request_raw([
            b"GET /status?foo=bar&baz=gaz#something HTTP/1.1"
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

    def test_send_request_with_headers(self):
        response = self.web_server.http_request_raw([
            b"GET /status HTTP/1.1",
            b"foo: bar",
            b"baz: gaz"
        ])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")
