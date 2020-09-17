from python.lib.testcase import KphpServerAutoTestCase


class TestSimpleScenarioExample(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--define": "hello=world"
        })

    def test_master_process_log(self):
        self.kphp_server.assert_log(["master"], "Can't find master process starting message")

    def test_uptime_stat(self):
        self.kphp_server.assert_stats({"kphp_server.uptime": self.cmpGe(1)})

    def test_send_request_check_log_and_stat(self):
        initial_stats = self.kphp_server.get_stats()

        resp = self.kphp_server.http_get()
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "Hello world!")

        self.kphp_server.assert_log(["worked"], "Can't find worker process request message")
        self.kphp_server.assert_stats({
            "kphp_server.requests_total_incoming_queries": self.cmpGe(1)
        }, initial_stats=initial_stats)
        self.assertKphpNoTerminatedRequests()

    def test_define(self):
        resp = self.kphp_server.http_get("/ini_get?hello")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "world")
