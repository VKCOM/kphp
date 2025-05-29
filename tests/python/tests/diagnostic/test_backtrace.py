import pytest
from python.lib.testcase import WebServerAutoTestCase


class TestBacktrace(WebServerAutoTestCase):
    @classmethod
    def extra_kphp2cpp_options(cls):
        return {"KPHP_EXTRA_CXXFLAGS" : "-O0"}

    def test_main_fork_trace(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "check_main_trace"},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)

    def test_fork_trace(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "check_fork_trace"},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)

    def test_fork_switch_trace(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "check_fork_switch_trace"},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)

    def test_nested_fork_trace(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "check_nested_fork_trace"},
            ])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)
