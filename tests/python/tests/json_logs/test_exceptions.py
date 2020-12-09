from python.lib.testcase import KphpServerAutoTestCase


class TestJsonLogsExceptions(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()

    def test_exception_no_context(self):
        resp = self.kphp_server.http_post(json=[{"op": "exception", "msg": "hello", "code": 123}])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 1, "env": "",  "tags": {"uncaught": True},
                "msg": "Unhandled exception from .+index.php:\\d+; Error 123; Message: hello",
            }])

    def test_exception_with_context(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "exception", "msg": "world", "code": 3456}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 1, "env": "efg",  "tags": {"a": "b", "uncaught": True}, "extra_info": {"c": "d"},
                "msg": "Unhandled exception from .+index.php:\\d+; Error 3456; Message: world",
            }])
