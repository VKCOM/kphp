import socket

from python.lib.testcase import KphpServerAutoTestCase
from python.lib.http_client import RawResponse


class TestFlush(KphpServerAutoTestCase):

    def test_one_flush(self):
        request = b"GET /test_script_flush?type=one_flush HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(request)
            first_chunk = RawResponse(s.recv(200))
            self.assertEqual(first_chunk.status_code, 200)
            self.assertEqual(first_chunk.content, b'Hello ')

            s.settimeout(None)
            second_chunk = s.recv(4096)
            self.assertEqual(second_chunk, b'world')

    def test_few_flush(self):
        request = b"GET /test_script_flush?type=few_flush HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(request)
            first_chunk = RawResponse(s.recv(200))
            self.assertEqual(first_chunk.status_code, 200)
            self.assertEqual(first_chunk.content, b'This ')

            second_chunk = s.recv(10)
            self.assertEqual(second_chunk, b'is big ')

            s.settimeout(None)
            third_chunk = s.recv(10)
            self.assertEqual(third_chunk, b'message')

    def test_transfer_encoding_chunked(self):
        request = b"GET /test_script_flush?type=transfer_encoding_chunked HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(request)
            first_chunk = RawResponse(s.recv(200))
            self.assertEqual(first_chunk.status_code, 200)
            self.assertEqual(first_chunk.headers["Transfer-Encoding"], "chunked")
            self.assertEqual(first_chunk.content, b'b\r\nHello world\r\n')

            s.settimeout(None)
            second_chunk = s.recv(4096)
            self.assertEqual(second_chunk, b'9\r\nBye world\r\n')

    def test_error_on_flush(self):
        request = b"GET /test_script_flush?type=error_on_flush HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(request)
            first_chunk = RawResponse(s.recv(200))
            self.assertEqual(first_chunk.status_code, 200)
            self.assertEqual(first_chunk.content, b'Start work')

            s.settimeout(None)
            second_chunk = s.recv(4096)
            self.kphp_server.assert_log(['Exception'], timeout=5)
            self.assertEqual(second_chunk, b'')
    def test_flush_and_header_register_callback_flush_inside_callback(self):
        request = b"GET /test_script_flush?type=flush_and_header_register_callback_flush_inside_callback HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(request)
            first_chunk = RawResponse(s.recv(200))
            self.assertEqual(first_chunk.status_code, 200)
            self.assertEqual(first_chunk.content, b'Zero One Two ')

            s.settimeout(None)
            second_chunk = s.recv(4096)
            self.assertEqual(second_chunk, b'Three ')

    def test_flush_and_header_register_callback_callback_after_flush(self):
        request = b"GET /test_script_flush?type=flush_and_header_register_callback_callback_after_flush HTTP/1.1\r\nHost:localhost\r\n\r\n"
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', self.kphp_server.http_port))
            s.send(request)
            first_chunk = RawResponse(s.recv(200))
            self.assertEqual(first_chunk.status_code, 200)
            self.assertEqual(first_chunk.content, b'Zero ')

            s.settimeout(None)
            second_chunk = s.recv(4096)
            self.assertEqual(second_chunk, b'One ')
