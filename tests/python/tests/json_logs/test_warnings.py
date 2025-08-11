import os
import socket
import pytest

from python.lib.testcase import WebServerAutoTestCase
from python.lib.kphp_server import KphpServer


class TestJsonLogsWarnings(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.ignore_log_errors()
        if cls.should_use_k2():
            cls.web_server.update_options({"--log-file": os.path.join(cls.web_server._working_dir, "data/log-file")})

    def test_warning_no_context(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "warning", "msg": "hello"},
                {"op": "warning", "msg": "world"}
            ])
        self.assertEqual(resp.text, "ok")
        expect_log = [
            {"msg": "hello", "env": ""},
            {"msg": "world", "env": ""}
        ] if self.should_use_k2() else [
            {"version": 0, "hostname": socket.gethostname(), "type": 2, "env": "", "msg": "hello", "tags": {"uncaught": False}},
            {"version": 0, "hostname": socket.gethostname(), "type": 2, "env": "", "msg": "world", "tags": {"uncaught": False}}
        ]
        self.web_server.assert_json_log(expect=expect_log)

    @pytest.mark.k2_skip
    def test_warning_with_special_chars(self):
        resp = self.web_server.http_post(json=[{"op": "warning", "msg": 'aaa"bbb"\nccc'}])
        self.assertEqual(resp.text, "ok")
        self.web_server.assert_json_log(
            expect=[{
                "version": 0, "hostname": socket.gethostname(), "type": 2, "env": "", "msg": "aaa\"bbb\" ccc",
                "tags": {"uncaught": False}
            }])

    def test_warning_with_tags(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "", "tags": {"a": "b"}, "extra_info": {}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        expect_log = [{
            "msg": "aaa", "env": "", "tags": {"a": "b"}
        }] if self.should_use_k2() else [{
            "version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "",
            "tags": {"uncaught": False, "a": "b"}
        }]
        self.web_server.assert_json_log(expect=expect_log)

    def test_warning_with_extra_info(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "", "tags": {}, "extra_info": {"a": "b"}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        expect_log = [{
            "msg": "aaa", "env": "", "extra_info": {"a": "b"}
        }] if self.should_use_k2() else [{
            "version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "",
            "tags": {"uncaught": False}, "extra_info": {"a": "b"}
        }]
        self.web_server.assert_json_log(expect=expect_log)

    def test_warning_with_env(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "abc", "tags": {}, "extra_info": {}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        expect_log = [{
            "msg": "aaa", "env": "abc"
        }] if self.should_use_k2() else [{
            "version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "abc",
            "tags": {"uncaught": False}
        }]
        self.web_server.assert_json_log(expect=expect_log)

    @pytest.mark.k2_skip
    def test_warning_with_env_special_chars(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "a\tb\nc/d\\e\x06f", "tags": {}, "extra_info": {}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        self.web_server.assert_json_log(
            expect=[{"version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "a b c/d\\e?f", "tags": {"uncaught": False}}])

    @pytest.mark.k2_skip
    def test_warning_with_long_env(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "x" * 128, "tags": {}, "extra_info": {}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        self.web_server.assert_json_log(
            expect=[{"version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "", "tags": {"uncaught": False}}])

    def test_warning_with_full_context(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        expect_log = [{
            "msg": "aaa", "env": "efg",
            "tags": {"a": "b"}, "extra_info": {"c": "d"}
        }] if self.should_use_k2() else [{
            "version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "efg",
            "tags": {"uncaught": False, "a": "b"}, "extra_info": {"c": "d"}
        }]
        self.web_server.assert_json_log(expect=expect_log)

    def test_warning_override_context(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "warning", "msg": "aaa"},
                {"op": "set_context", "env": "", "tags": {}, "extra_info": {}},
                {"op": "warning", "msg": "bbb"},
                {"op": "set_context", "env": "xxx", "tags": {}, "extra_info": {}},
                {"op": "warning", "msg": "ccc"},
                {"op": "set_context", "env": "", "tags": {"aa": "bb"}, "extra_info": {}},
                {"op": "warning", "msg": "ddd"},
                {"op": "set_context", "env": "", "tags": {}, "extra_info": {"cc": "dd"}},
                {"op": "warning", "msg": "eee"},
            ])
        self.assertEqual(resp.text, "ok")
        expect_log = [
            {"msg": "aaa", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
            {"msg": "bbb", "env": ""},
            {"msg": "ccc", "env": "xxx"},
            {"msg": "ddd", "env": "", "tags": {"aa": "bb"}},
            {"msg": "eee", "env": "", "extra_info": {"cc": "dd"}}
        ] if self.should_use_k2() else [
            {
                "version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "efg",
                "tags": {"uncaught": False, "a": "b"}, "extra_info": {"c": "d"}
            },
            {"version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "bbb", "env": "", "tags": {"uncaught": False}},
            {"version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "ccc", "env": "xxx", "tags": {"uncaught": False}},
            {"version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "ddd", "env": "", "tags": {"uncaught": False, "aa": "bb"}},
            {
                "version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "eee", "env": "",
                "tags": {"uncaught": False}, "extra_info": {"cc": "dd"}
            }
        ]
        self.web_server.assert_json_log(expect=expect_log)

    def test_warning_vector_context(self):
        resp = self.web_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": ["a", "b"], "extra_info": ["c", "d"]},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        expect_log = [{
            "msg": "aaa", "env": "efg",
            "tags": {"0": "a", "1": "b"}, "extra_info": {"0": "c", "1": "d"}
        }] if self.should_use_k2() else [{
            "version": 0, "hostname": socket.gethostname(), "type": 2, "msg": "aaa", "env": "efg",
            "tags": {"uncaught": False, "0": "a", "1": "b"}, "extra_info": {"0": "c", "1": "d"}
        }]
        self.web_server.assert_json_log(expect=expect_log, timeout=1)

    @pytest.mark.k2_skip
    def test_error_tag_context(self):
        if isinstance(self.web_server, KphpServer):
            self.web_server.set_error_tag(100500)
        self.web_server.restart()
        resp = self.web_server.http_post(json=[{"op": "warning", "msg": "hello"}])
        self.assertEqual(resp.text, "ok")
        self.web_server.assert_json_log(
            expect=[{
                "version": 100500, "hostname": socket.gethostname(), "type": 2, "env": "", "msg": "hello", "tags": {"uncaught": False}
            }])
        if isinstance(self.web_server, KphpServer):
            self.web_server.set_error_tag(None)
        self.web_server.restart()
