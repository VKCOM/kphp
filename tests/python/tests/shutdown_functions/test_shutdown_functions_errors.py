from python.lib.testcase import KphpServerAutoTestCase


class TestShutdownFunctionsErrors(KphpServerAutoTestCase):
    def test_critical_error(self):
        # with self.assertRaises(RuntimeError):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_critical_error"},
            ])
        self.assertEqual(resp.text, "ERROR")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_log([
            "running shutdown handler critical errors",
            "critical error from shutdown function",
        ], timeout=5)
