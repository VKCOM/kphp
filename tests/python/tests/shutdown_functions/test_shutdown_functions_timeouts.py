from python.lib.testcase import KphpServerAutoTestCase


class TestShutdownFunctionsTimeouts(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--time-limit": 1
        })

    def test_timeout_reset_at_shutdown_function(self):
        # test that the timeout timer resets, giving the shutdown functions a chance to complete
        # if they're executed *before* the timeout but after a long-running script
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_after_long_work"},
                {"op": "sleep", "duration": 0.75},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)
        self.kphp_server.assert_log(["shutdown function managed to finish"], timeout=5)

    def test_timeout_shutdown_exit(self):
        # test that if we're doing an exit(0) in shutdown handler *after* the timeout
        # that request will still end up in error state with 500 status code
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_with_exit"},
                {"op": "long_work", "duration": 2}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_log([
            "Critical error during script execution: timeout exit",
            "running shutdown handler 1"
        ], timeout=5)

    def test_timeout_after_timeout_at_shutdown_function(self):
        # test that we do set up a second timeout for the shutdown functions
        # that were executed *after* the (first) timeout
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_endless_loop"},
                {"op": "long_work", "duration": 2}
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_log(["Critical error during script execution: timeout exit"], timeout=5)
