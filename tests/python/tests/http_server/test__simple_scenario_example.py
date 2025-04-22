import pytest
from python.lib.testcase import WebServerAutoTestCase


class TestSimpleScenarioExample(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if not cls.should_use_k2():
            # For K2 case ini defines are declared in component-config.yaml
            cls.web_server.update_options({
                "--define": "hello=world"
            })

    @pytest.mark.k2_skip
    def test_master_process_log(self):
        self.web_server.restart()
        self.web_server.assert_log(["master"], "Can't find master process starting message")

    @pytest.mark.k2_skip
    def test_uptime_stat(self):
        self.web_server.assert_stats({"kphp_server.uptime": self.cmpGe(1)})

    @pytest.mark.k2_skip
    def test_send_request_check_log_and_stat(self):
        initial_stats = self.web_server.get_stats()

        resp = self.web_server.http_get()
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "Hello world!")

        self.web_server.assert_log(["worked"], "Can't find worker process request message")
        self.web_server.assert_stats({
            "kphp_server.workers_general_requests_total_incoming_queries": self.cmpGe(1)
        }, initial_stats=initial_stats)
        self.assertKphpNoTerminatedRequests()

    def test_define(self):
        resp = self.web_server.http_get("/ini_get?hello")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "world")
