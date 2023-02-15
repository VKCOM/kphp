from python.lib.testcase import KphpServerAutoTestCase

class TestShutdownFunctionsNetErrors(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()
        cls.kphp_server.update_options({
            "--time-limit": 1
        })

    def test_timeout_in_net_context(self):
        # test that the timeout timer will go off in net context
        # shutdown function will be called
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_trivial"},
                {"op": "fork"}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_json_log(
            expect=[{"version": 0, "type": 2, "env": "", "msg": "trivial shutdown function work", "tags": {"uncaught": False}}],
            timeout=5)

    def test_connection_close(self):
        # test that if the http connection is closed
        # shutdown function will be called
        try:
            self.kphp_server.http_post(
                json=[
                    {"op": "register_shutdown_function", "msg": "shutdown_trivial"},
                    {"op": "fork"}
                ],
                timeout=0.5)
        except Exception:
            pass
        self.kphp_server.assert_json_log(
            expect=[{"version": 0, "type": 2, "env": "", "msg": "trivial shutdown function work", "tags": {"uncaught": False}}],
            timeout=5)
