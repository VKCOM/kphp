import socket

from python.lib.testcase import KphpServerAutoTestCase


class TestJsonLogTimeouts(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()
        cls.kphp_server.update_options({
            "--time-limit": 1
        })

    def test_timeout_no_context(self):
        resp = self.kphp_server.http_post(json=[{"op": "long_work", "duration": 2}])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 1, "env": "", "tags": {"uncaught": True},
                "msg": "Maximum execution time exceeded",
            }],
            timeout=5)

    def test_timeout_with_context(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "long_work", "duration": 2}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 1, "env": "efg", "tags": {"a": "b", "uncaught": True}, "extra_info": {"c": "d"},
                "msg": "Maximum execution time exceeded",
            }],
            timeout=5)
