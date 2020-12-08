from python.lib.testcase import KphpServerAutoTestCase


class TestJsonLogsWarnings(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()

    def test_warning_no_context(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "warning", "msg": "hello"},
                {"op": "warning", "msg": "world"}
            ])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[
                {"version": 0, "type": 2, "env": "", "msg": "hello", "tags": {"uncaught": False}},
                {"version": 0, "type": 2, "env": "", "msg": "world", "tags": {"uncaught": False}}
            ])

    def test_warning_with_special_chars(self):
        resp = self.kphp_server.http_post(json=[{"op": "warning", "msg": 'aaa"bbb"\nccc'}])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 2, "env": "", "msg": "aaa'bbb' ccc",
                "tags": {"uncaught": False}
            }])

    def test_warning_with_tags(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "", "tags": {"a": "b"}, "extra_info": {}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 2, "msg": "aaa", "env": "",
                "tags": {"uncaught": False, "a": "b"}
            }])

    def test_warning_with_extra_info(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "", "tags": {}, "extra_info": {"a": "b"}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 2, "msg": "aaa", "env": "",
                "tags": {"uncaught": False}, "extra_info": {"a": "b"}
            }])

    def test_warning_with_env(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "abc", "tags": {}, "extra_info": {}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[{"version": 0, "type": 2, "msg": "aaa", "env": "abc", "tags": {"uncaught": False}}])

    def test_warning_with_full_context(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 2, "msg": "aaa", "env": "efg",
                "tags": {"uncaught": False, "a": "b"}, "extra_info": {"c": "d"}
            }])

    def test_warning_override_context(self):
        resp = self.kphp_server.http_post(
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
        self.kphp_server.assert_json_log(
            expect=[
                {
                    "version": 0, "type": 2, "msg": "aaa", "env": "efg",
                    "tags": {"uncaught": False, "a": "b"}, "extra_info": {"c": "d"}
                },
                {"version": 0, "type": 2, "msg": "bbb", "env": "", "tags": {"uncaught": False}},
                {"version": 0, "type": 2, "msg": "ccc", "env": "xxx", "tags": {"uncaught": False}},
                {"version": 0, "type": 2, "msg": "ddd", "env": "", "tags": {"uncaught": False, "aa": "bb"}},
                {
                    "version": 0, "type": 2, "msg": "eee", "env": "",
                    "tags": {"uncaught": False}, "extra_info": {"cc": "dd"}
                }
            ])

    def test_warning_vector_context(self):
        resp = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "efg", "tags": ["a", "b"], "extra_info": ["c", "d"]},
                {"op": "warning", "msg": "aaa"},
            ])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 2, "msg": "aaa", "env": "efg",
                "tags": {"uncaught": False, "0": "a", "1": "b"}, "extra_info": {"0": "c", "1": "d"}
            }], timeout=1)

    def test_error_tag_context(self):
        self.kphp_server.set_error_tag(100500)
        self.kphp_server.restart()
        resp = self.kphp_server.http_post(json=[{"op": "warning", "msg": "hello"}])
        self.assertEqual(resp.text, "ok")
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 100500, "type": 2, "env": "", "msg": "hello", "tags": {"uncaught": False}
            }])
        self.kphp_server.set_error_tag(None)
        self.kphp_server.restart()
