import pytest
from python.lib.testcase import WebServerAutoTestCase


class TestBacktrace(WebServerAutoTestCase):
    @classmethod
    def extra_kphp2cpp_options(cls):
        return {"KPHP_EXTRA_CXXFLAGS" : "-O0 -g"}
    
    def query(self, op: str):
        resp = self.web_server.http_post(uri="/test_backtrace", json=[{"op": op}])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)

    def test_main_fork_trace(self):
        self.query("check_main_trace")

    def test_fork_trace(self):
        self.query("check_fork_trace")

    def test_fork_switch_trace(self):
        self.query("check_fork_switch_trace")

    def test_nested_fork_trace(self):
        self.query("check_nested_fork_trace")
