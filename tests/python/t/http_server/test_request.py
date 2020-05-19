import urllib

from python.lib.testcase import KphpServerAutoTestCase


class TestRequest(KphpServerAutoTestCase):
    def test_send_simple_request_path(self):
        response = self.kphp_server.http_request_raw([b"GET /status HTTP/1.1"])
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"Hello world!")

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

    def test_send_request_with_fragmet(self):
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
