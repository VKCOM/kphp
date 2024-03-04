from python.lib.testcase import KphpServerAutoTestCase


class TestShutdownFunctions(KphpServerAutoTestCase):
    def test_fork_wait(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "register_shutdown_function", "msg": "shutdown_fork_wait"},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)
        self.kphp_server.assert_log([
            "before fork",
            "after fork",
            "before yield",
            "after yield",
            "after wait",
        ], timeout=5)
