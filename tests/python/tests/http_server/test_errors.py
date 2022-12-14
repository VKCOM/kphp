from python.lib.testcase import KphpServerAutoTestCase


class TestErrors(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 1
        })

    def test_script_errors(self):
        stats_before = self.kphp_server.get_stats()

        for i in range(5):
            response = self.kphp_server.http_request(uri="/test_script_errors", method='GET')
            self.assertEqual(500, response.status_code)
            self.assertTrue(len(response.content) > 0)
            self.kphp_server.assert_log([
                'Warning: Critical error "Test error" in file',
                "Critical error during script execution: php assert error"
            ])

        self.kphp_server.assert_stats(
            timeout=5,
            initial_stats=stats_before,
            expected_added_stats={
                # ensure shared data is not leaked
                "kphp_server.server_workers_strange_dead": 0,
                "kphp_server.server_workers_killed": 0,
                "kphp_server.server_workers_failed": 0,
                "kphp_server.server_workers_dead": 0,
                "kphp_server.server_workers_terminated": 0,
            })
