from python.lib.testcase import KphpServerAutoTestCase


class TestShutdownFunctionsExceptions(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()

    def test_exception_shutdown_function(self):
        # test that:
        # 1. we execute shutdown functions after the "uncaught exception" critical error
        # 2. that CurException is not visible inside shutdown functions
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_exception_warning"},
                {"op": "exception", "msg": "hello", "code": 123},
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_json_log(
            expect=[
                {
                    "version": 0, "type": 1, "env": "",  "tags": {"uncaught": True},
                    "msg": "Unhandled ServerException from index.php:\\d+; Error 123; Message: hello",
                },
                {"version": 0, "type": 2, "env": "", "msg": "running shutdown handler", "tags": {"uncaught": False}},
            ])

    def test_exception_and_exit_shutdown_function(self):
        # test that:
        # We correctly execute shutdown functions with the exit function inside after exception
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_with_exit"},
                {"op": "exception", "msg": "hello", "code": 123},
            ])
        self.assertEqual(resp.status_code, 200)
        self.kphp_server.assert_json_log(
            expect=[
                {"version": 0, "type": 2, "env": "", "msg": "running shutdown handler 1", "tags": {"uncaught": False}},])
        correct = False
        try:
            self.kphp_server.assert_json_log(
                expect=[
                    {"version": 0, "type": 2, "env": "", "msg": "running shutdown handler 1", "tags": {"uncaught": False}},],
                timeout=1)
        except Exception:
            correct = True
        self.assertTrue(correct)


