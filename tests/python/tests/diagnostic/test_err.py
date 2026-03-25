import pytest
from python.lib.testcase import WebServerAutoTestCase


class TestErr(WebServerAutoTestCase):
    def query(self, op: str):
        resp = self.web_server.http_post(uri="/test_err", json=[{"op": op}])
        self.assertEqual(resp.text, "ok")
        self.assertEqual(resp.status_code, 200)

    def test_just_make(self):
        self.query("just_make")

    def test_throw_catch(self):
        self.query("throw_catch")
