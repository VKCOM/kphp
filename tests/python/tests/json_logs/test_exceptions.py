import socket
import pytest

from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestJsonLogsExceptions(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.ignore_log_errors()

    def test_exception_no_context(self):
        resp = self.web_server.http_post(json=[{"op": "exception", "msg": "hello", "code": 123}])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.web_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 1, "env": "", "tags": {"uncaught": True},
                "msg": "Unhandled ServerException from index.php:\\d+; Error 123; Message: hello",
            }])

    def test_exception_with_context(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "exception", "msg": "world", "code": 3456}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.web_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 1, "env": "efg", "tags": {"a": "b", "uncaught": True}, "extra_info": {"c": "d"},
                "msg": "Unhandled ServerException from index.php:\\d+; Error 3456; Message: world",
            }])

    def test_exception_with_special_chars(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b\\c\"d\n"}, "extra_info": {"c": "\\\\xxx\""}},
                {"op": "exception", "msg": "\\a\\b\n\tc\"d \x06", "code": 123}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.web_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 1, "env": "efg", "tags": {"a": "b\\c\"d\n", "uncaught": True},
                "extra_info": {"c": "\\\\xxx\""},
                "msg": "Unhandled ServerException from index.php:\\d+; Error 123; Message: \\\\a\\\\b  c\"d ?",
            }])
