import socket
from python.lib.testcase import KphpServerAutoTestCase
from python.lib.http_client import RawResponse

class TestHeadersSent(KphpServerAutoTestCase):
    def test(self):
        request = b"GET /test_headers_sent HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(request)

            self.assertEqual(b"0", RawResponse(s.recv(4096)).content)
            self.assertEqual(b"1", s.recv(20))
