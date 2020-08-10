from .kphp_with_confdata import KphpWithConfdata


class TestConfdataDefault(KphpWithConfdata):
    CONFDATA = {
        "some_key0": "some_value0",
        "some_key1": "some_value1",
        "some_key2": "some_value2",
        "one_dot.key1": "one_dot_value1",
        "one_dot.key2": "one_dot_value2",
        "two.dots.key1": "two_dots_value1",
        "two.dots.key2": "two_dots_value2",
        "three.dots.key.1": "three_dots_value_2",
        "three.dots.key.2": "three_dots_value_1"
    }

    def test_confdata_stats(self):
        self.kphp_server.assert_stats(
            prefix="kphp_server.confdata_",
            expected_added_stats={
                "memory_limit": self.cmpGt(0),
                "memory_used": self.cmpGt(0),
                "memory_used_max": self.cmpGt(0),
                "memory_real_used": self.cmpGt(0),
                "memory_real_used_max": self.cmpGt(0),
                "memory_defragmentation_calls": self.cmpGe(0),
                "memory_huge_memory_pieces": self.cmpGe(0),
                "memory_small_memory_pieces": self.cmpGe(0),
                "initial_loading_duration": self.cmpGe(0),
                "total_updating_time": self.cmpGe(0),
                "seconds_since_last_update": self.cmpGe(0),
                "updates_ignored": 0,
                "updates_total": self.cmpGt(0),
                "elements_total": len(self.CONFDATA),
                "elements_simple_key": 3,
                "elements_one_dot_wildcard": 6,
                "elements_two_dots_wildcard": 4,
                "elements_predefined_wildcard": 0,
                "elements_with_delay": 0,
                "wildcards_one_dot": 3,
                "wildcards_two_dots": 2,
                "wildcards_predefined": 0,
                "vars_in_garbage_last": self.cmpGe(0),
                "vars_in_garbage_last_100_max": self.cmpGe(0),
                "vars_in_garbage_last_100_avg": self.cmpGe(0),
                "binlog_events_set_forever_total": len(self.CONFDATA),
            })

    def test_get_values_warning(self):
        r = self._call_function("get_value", "")
        self.assertIsNone(r)
        self.kphp_server.assert_log(["Warning: Empty key is not supported"])

    def test_get_values_by_any_wildcard_warning(self):
        r = self._call_function("get_values_by_any_wildcard", "")
        self.assertEqual(r, [])
        self.kphp_server.assert_log(["Warning: Empty wildcard is not supported"])

    def test_get_values_by_predefined_wildcard_warning(self):
        r = self._call_function("get_values_by_predefined_wildcard", "")
        self.assertEqual(r, [])
        self.kphp_server.assert_log(["Warning: Empty wildcard is not supported"])

        for key in ("foo", "hello.world", "...", "some_", "one_dot.key", "two", "three.dots.key", "three.dots.key."):
            r = self._call_function("get_values_by_predefined_wildcard", key)
            self.assertEqual(r, [])
            self.kphp_server.assert_log(["Warning: Trying to get elements by non predefined wildcard '{}'".format(key)])

    def test_get_value(self):
        for key, value in self.CONFDATA.items():
            self.assertEqual(self._call_function("get_value", key), value)

        for key in ("unkown_key", "some_key", "one_dot.", "one_dot.key", "one_dot.key3", "three.dots.key"):
            self.assertIsNone(self._call_function("get_value", key))

    def _do_test_for_predefined(self, function):
        for key in ("non_existed.", "non.existed.", ".", ".."):
            r = self._call_function(function, key)
            self.assertEqual(r, [])

        r = self._call_function(function, "one_dot.")
        self.assertEqual(r, {
            "key1": "one_dot_value1",
            "key2": "one_dot_value2"
        })

        r = self._call_function(function, "two.")
        self.assertEqual(r, {
            "dots.key1": "two_dots_value1",
            "dots.key2": "two_dots_value2"
        })

        r = self._call_function(function, "two.dots.")
        self.assertEqual(r, {
            "key1": "two_dots_value1",
            "key2": "two_dots_value2"
        })

        r = self._call_function(function, "three.")
        self.assertEqual(r, {
            "dots.key.1": "three_dots_value_2",
            "dots.key.2": "three_dots_value_1"
        })

        r = self._call_function(function, "three.dots.")
        self.assertEqual(r, {
            "key.1": "three_dots_value_2",
            "key.2": "three_dots_value_1"
        })

    def test_get_values_by_predefined_wildcard(self):
        self._do_test_for_predefined("get_values_by_predefined_wildcard")

    def test_get_values_by_any_wildcard(self):
        self._do_test_for_predefined("get_values_by_any_wildcard")

        for key in ("abc", "non.existed", "..."):
            r = self._call_function("get_values_by_any_wildcard", key)
            self.assertEqual(r, [])

        r = self._call_function("get_values_by_any_wildcard", "some_key0")
        self.assertEqual(r, {"": "some_value0"})

        r = self._call_function("get_values_by_any_wildcard", "some_")
        self.assertEqual(r, {
            "key0": "some_value0",
            "key1": "some_value1",
            "key2": "some_value2"
        })

        r = self._call_function("get_values_by_any_wildcard", "some_key")
        self.assertEqual(r, [
            "some_value0",
            "some_value1",
            "some_value2"
        ])

        r = self._call_function("get_values_by_any_wildcard", "one_do")
        self.assertEqual(r, {
            "t.key1": "one_dot_value1",
            "t.key2": "one_dot_value2"
        })

        r = self._call_function("get_values_by_any_wildcard", "one_dot.key")
        self.assertEqual(r, {
            "1": "one_dot_value1",
            "2": "one_dot_value2"
        })

        r = self._call_function("get_values_by_any_wildcard", "two")
        self.assertEqual(r, {
            ".dots.key1": "two_dots_value1",
            ".dots.key2": "two_dots_value2"
        })

        r = self._call_function("get_values_by_any_wildcard", "two.dot")
        self.assertEqual(r, {
            "s.key1": "two_dots_value1",
            "s.key2": "two_dots_value2"
        })

        r = self._call_function("get_values_by_any_wildcard", "three.dot")
        self.assertEqual(r, {
            "s.key.1": "three_dots_value_2",
            "s.key.2": "three_dots_value_1"
        })

        r = self._call_function("get_values_by_any_wildcard", "three.dots.key.")
        self.assertEqual(r, {
            "1": "three_dots_value_2",
            "2": "three_dots_value_1"
        })
