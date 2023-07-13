from python.lib.testcase import KphpServerAutoTestCase


class TestShutdownFunctionsTimeouts(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--time-limit": 1,
            "--verbosity-resumable=2": True,
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
                {"op": "long_work", "duration": 1.5}
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
        self.kphp_server.assert_log(["Critical error during script execution: hard timeout exit"], timeout=5)

    # TODO enable shutdown functions call if timeout occurs in net context
    # def test_timeout_from_resumable_in_main_thread(self):
    #     # tests that when timeout is raised inside some 'started' resumable in main thread
    #     # and shutdown functions run, fork switching and resumables handling works correctly
    #     resp = self.kphp_server.http_post(
    #         json=[
    #             {"op": "register_shutdown_function", "msg": "shutdown_fork_wait"},
    #             {"op": "resumable_long_work", "duration": 2},
    #         ])
    #     self.assertEqual(resp.text, "ERROR")
    #     self.assertEqual(resp.status_code, 500)
    #     self.kphp_server.assert_log(["Critical error during script execution: timeout",
    #                                  "shutdown_fork_wait\\(\\): running_fork_id=0",
    #                                  "before fork",
    #                                  "after fork",
    #                                  "before yield",
    #                                  "after yield",
    #                                  "after wait",
    #                                  ], timeout=5)
    #
    # def test_timeout_from_fork(self):
    #     # tests that when timeout is raised inside some fork
    #     # and shutdown functions run, fork switching and resumables handling works correctly
    #     resp = self.kphp_server.http_post(
    #         json=[
    #             {"op": "register_shutdown_function", "msg": "shutdown_fork_wait"},
    #             {"op": "fork_wait_resumable_long_work", "duration": 2},
    #         ])
    #     self.assertEqual(resp.text, "ERROR")
    #     self.assertEqual(resp.status_code, 500)
    #     self.kphp_server.assert_log(["Critical error during script execution: timeout",
    #                                  "shutdown_fork_wait\\(\\): running_fork_id=0",
    #                                  "before fork",
    #                                  "after fork",
    #                                  "before yield",
    #                                  "after yield",
    #                                  "after wait",
    #                                  ], timeout=5)
    #
    # def test_resume_script_resumable(self):
    #     #tests that answers after timeout doesn't break anything, ad doesn't continue script resumables
    #     resp = self.kphp_server.http_post(
    #         json=[
    #             {"op": "register_shutdown_function", "msg": "shutdown_fork_wait"},
    #             {"op": "sleep", "duration": 0.7},
    #             {"op": "fork_send_rpc_without_wait", "duration": 0.5, "master_port": self.kphp_server.master_port},
    #             {"op": "long_work", "duration": 2}
    #         ])
    #     self.assertEqual(resp.text, "ERROR")
    #     self.assertEqual(resp.status_code, 500)
    #     self.kphp_server.assert_log(["RPC request sent successfully = 1",
    #                                  "Critical error during script execution: timeout",
    #                                  "shutdown_fork_wait\\(\\): running_fork_id=0",
    #                                  "before fork",
    #                                  "after fork",
    #                                  "before yield",
    #                                  "after yield",
    #                                  "after wait",
    #                                  ], timeout=5)
    #
    # def test_timeout_in_net_context(self):
    #     #tests that script execute shutdown functions if timeout was in net context
    #     resp = self.kphp_server.http_post(
    #         json=[
    #             {"op": "register_shutdown_function", "msg": "shutdown_after_long_work"},
    #             {"op": "send_long_rpc", "duration": 1, "master_port": self.kphp_server.master_port},
    #         ])
    #     self.assertEqual(resp.text, "ERROR")
    #     self.assertEqual(resp.status_code, 500)
    #     self.kphp_server.assert_log(["Critical error during script execution: timeout",
    #                                  "shutdown function managed to finish"], timeout=5)
