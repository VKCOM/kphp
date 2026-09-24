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

    def test_response_headers_grow_beyond_one_megabyte(self):
        response = self.response('/large-headers')
        for i in range(24):
            self.assertEqual(response.getheader(f'x-large-{i}'), 'x' * 49152)
        self.assertEqual(response.read(), b'large headers')

    def test_content_length_fully_flushed_before_script_finishes(self):
        for size in [0, 5]:
            with self.subTest(size=size):
                conn = self.connect()
                conn.request('GET', f'/content-length-flushed?size={size}')
                response = conn.getresponse()
                self.assertEqual(response.status, 200)
                self.assertEqual(response.read(), b'first' if size else b'')
                self.web_server.assert_log([f'content-length completed {size}'], timeout=1)
                conn.request('GET', '/empty')
                self.assertEqual(conn.getresponse().read(), b'')

    def test_ignore_user_abort_during_flush(self):
        conn = self.connect()
        conn.request('GET', '/ignore-abort-flush')
        response = conn.getresponse()
        self.assertEqual(response.read(5), b'first')
        response.close()
        conn.close()
        self.web_server.assert_log([r'Error serving connection: .*', 'ignore-abort flush completed'], timeout=1)

    def test_user_abort_during_flush_runs_shutdown(self):
        conn = self.connect()
        conn.request('GET', '/abort-flush')
        response = conn.getresponse()
        self.assertEqual(response.read(5), b'first')
        response.close()
        conn.close()
        self.web_server.assert_log([r'Error serving connection: .*', 'abort flush shutdown completed'], timeout=1)
        self.assertFalse(any('ignore-abort flush completed' in line for line in self.web_server.get_log()))

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

    def test_concurrent_flush_waits_for_header_callback(self):
        for path in ['/callback-concurrent', '/callback-preexisting']:
            with self.subTest(path=path):
                response = self.response(path)
                self.assertEqual(response.getheader('x-callback'), 'ready')
                self.assertEqual(response.read(), b'callback-done')

    def test_header_callback_descendants_can_flush(self):
        for path in ['/callback-descendants', '/callback-descendants-final']:
            with self.subTest(path=path):
                response = self.response(path)
                self.assertEqual(response.getheader('x-callback'), 'ready')
                self.assertIsNone(response.getheader('x-too-late'))
                self.assertEqual(response.read(), b'child-after')

    def test_header_callback_flush_releases_waiting_fork(self):
        response = self.response('/callback-release-waiter')
        self.assertEqual(response.getheader('x-callback'), 'ready')
        self.assertEqual(response.read(), b'first-after')

    def test_finalization_waits_for_running_header_callback(self):
        for path in ['/callback-finalize-running', '/callback-finalize-committed']:
            with self.subTest(path=path):
                response = self.response(path)
                self.assertEqual(response.getheader('x-callback'), 'ready')
                self.assertEqual(response.read(), b'first-last')

    def test_exit_inside_header_callback(self):
        for path in ['/callback-exit-flush', '/callback-exit-final']:
            with self.subTest(path=path):
                response = self.response(path)
                self.assertEqual(response.getheader('x-callback'), 'ready')
                self.assertEqual(response.read(), b'exit')

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

    def test_compressed_binary_chunks(self):
        data = bytearray()
        random_state = 1
        for _ in range(65536):
            random_state = (random_state * 1103515245 + 12345) & 0x7fffffff
            data.append((random_state >> 16) & 255)
        expected = b''.join(data[:size] for size in [0, 1, 7, 8, 31, 32, 255, 256, 16383, 65536])
        for encoding, window in [('gzip', 31), ('deflate', 15)]:
            with self.subTest(encoding=encoding):
                response = self.response('/gzip-binary', encoding)
                self.assertEqual(response.getheader('content-encoding'), encoding)
                decoder = zlib.decompressobj(window)
                self.assertEqual(decoder.decompress(response.read()), expected)
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

    def test_suppressed_body_allows_script_to_finish(self):
        for method, status in [('HEAD', 200), ('GET', 204), ('GET', 304)]:
            with self.subTest(method=method, status=status):
                conn = self.connect()
                conn.request(method, f'/no-body?status={status}')
                response = conn.getresponse()
                self.assertEqual(response.status, status)
                self.assertEqual(response.read(), b'')
                self.web_server.assert_log([f'no-body completed {status}'])

    def test_content_length_with_flush_and_keepalive(self):
        conn = self.connect()
        conn.request('GET', '/content-length')
        response = conn.getresponse()
        self.assertEqual(response.getheader('content-length'), '10')
        self.assertEqual(response.read(), b'first-last')
        conn.request('GET', '/empty')
        self.assertEqual(conn.getresponse().read(), b'')


@pytest.mark.kphp_skip_suite
class TestFlushTokio(TestFlush):
    @classmethod
    def extra_class_setup(cls):
        super().extra_class_setup()
        cls.web_server.update_options({'--use-tokio-executor': True})
