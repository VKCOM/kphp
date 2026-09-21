import http.client
import time
import zlib

import pytest

from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.kphp_skip_suite
class TestFlush(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({'--http-request-timeout': 2})

    def connect(self):
        conn = http.client.HTTPConnection('127.0.0.1', self.web_server.http_port, timeout=5)
        self.addCleanup(conn.close)
        return conn

    def response(self, path, encoding='identity', method='GET'):
        conn = self.connect()
        conn.request(method, path, headers={'Accept-Encoding': encoding})
        response = conn.getresponse()
        self.assertEqual(response.status, 200)
        return response

    def test_streaming_before_script_finishes(self):
        response = self.response('/stream')
        self.assertEqual(response.read(6), b'first\n')
        first = time.monotonic()
        self.assertEqual(response.read(), b'last\n')
        self.assertGreater(time.monotonic() - first, 0.2)

    def test_headers_are_committed(self):
        response = self.response('/headers')
        self.assertEqual(response.getheader('x-before'), 'yes')
        self.assertIsNone(response.getheader('x-after'))
        self.assertEqual(response.read(), b'010')

    def test_flush_in_header_callback(self):
        for path in ['/callback-final', '/callback-flush']:
            with self.subTest(path=path):
                self.assertEqual(self.response(path).read(), b'before-callback-after')

    def test_compressed_chunks_form_one_stream(self):
        for encoding, window in [('gzip', 31), ('deflate', 15)]:
            for path in ['/gzip', '/gzip-close']:
                with self.subTest(encoding=encoding, path=path):
                    response = self.response(path, encoding)
                    self.assertEqual(response.getheader('content-encoding'), encoding)
                    decoder = zlib.decompressobj(window)
                    self.assertEqual(decoder.decompress(response.read()), b'first-last')
                    self.assertTrue(decoder.eof)
                    self.assertEqual(decoder.unused_data, b'')

    def test_late_gzip_does_not_change_wire_encoding(self):
        response = self.response('/late-gzip', 'gzip')
        self.assertIsNone(response.getheader('content-encoding'))
        self.assertEqual(response.read(), b'first-last')

    def test_user_buffers_are_only_drained_at_finalization(self):
        self.assertEqual(self.response('/buffers').read(), b'system-user-nested')

    def test_concurrent_forks_preserve_identity_and_body(self):
        self.assertEqual(self.response('/forks').read(), b'A' * 100000 + b'B' * 100000)

    def test_large_body_with_slow_reader(self):
        response = self.response('/large')
        time.sleep(0.1)
        self.assertEqual(response.read(), b'x' * (16 * 65536))

    def test_timeout_is_not_successful_eof(self):
        response = self.response('/timeout')
        self.assertEqual(response.read(5), b'first')
        with self.assertRaises(http.client.IncompleteRead):
            response.read()
        self.web_server.assert_log([r'Error serving connection: .*HTTP response interrupted'])

    def test_empty_flush_and_head(self):
        self.assertEqual(self.response('/empty').read(), b'')
        self.assertEqual(self.response('/gzip', 'gzip', 'HEAD').read(), b'')


@pytest.mark.kphp_skip_suite
class TestFlushTokio(TestFlush):
    @classmethod
    def extra_class_setup(cls):
        super().extra_class_setup()
        cls.web_server.update_options({'--use-tokio-executor': True})
