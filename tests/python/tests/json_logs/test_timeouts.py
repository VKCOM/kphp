import socket
import pytest

from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestJsonLogTimeouts(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.ignore_log_errors()
        cls.web_server.update_options({
            "--time-limit": 1
        })

    def test_timeout_no_context(self):
        resp = self.web_server.http_post(json=[{"op": "long_work", "duration": 2}])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.web_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 1, "env": "", "tags": {"uncaught": True},
                "msg": "Maximum execution time exceeded",
            }],
            timeout=5)

    def test_timeout_with_context(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "long_work", "duration": 2}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.web_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 1, "env": "efg", "tags": {"a": "b", "uncaught": True}, "extra_info": {"c": "d"},
                "msg": "Maximum execution time exceeded",
            }],
            timeout=5)
