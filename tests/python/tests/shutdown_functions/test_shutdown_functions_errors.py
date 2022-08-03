from python.lib.testcase import KphpServerAutoTestCase


class TestShutdownFunctionsErrors(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()

    def test_null_interface_method_call_error(self):
        # See #569
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_exception_warning"},
                {"op": "null_interface_method_call"},
            ])
        self.assertEqual(resp.status_code, 200)
        self.kphp_server.assert_json_log(
            expect=[
                {"version": 0, "type": 1, "env": "", "msg": "call method\\(Iface::f\\) on null object", "tags": {"uncaught": True}},
                {"version": 0, "type": 2, "env": "", "msg": "running shutdown handler", "tags": {"uncaught": False}},
            ],
            timeout=5)

    def test_critical_error(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_critical_error"},
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_json_log(
            expect=[
                {"version": 0, "type": 2, "env": "", "msg": "running shutdown handler critical errors", "tags": {"uncaught": False}},
                {"version": 0, "type": 1, "env": "", "msg": "critical error from shutdown function", "tags": {"uncaught": True}},
            ])
