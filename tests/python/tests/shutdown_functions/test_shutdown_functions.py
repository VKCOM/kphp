from python.lib.testcase import KphpServerAutoTestCase


class TestShutdownFunctions(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()

    def test_fork_wait(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_fork_wait"},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)
        self.kphp_server.assert_json_log(
            expect=[
                {"version": 0, "type": 2, "env": "", "msg": "before fork", "tags": {"uncaught": False}},
                {"version": 0, "type": 2, "env": "", "msg": "after fork", "tags": {"uncaught": False}},
                {"version": 0, "type": 2, "env": "", "msg": "before yield", "tags": {"uncaught": False}},
                {"version": 0, "type": 2, "env": "", "msg": "after yield", "tags": {"uncaught": False}},
                {"version": 0, "type": 2, "env": "", "msg": "after wait", "tags": {"uncaught": False}},
            ],
            timeout=5)
