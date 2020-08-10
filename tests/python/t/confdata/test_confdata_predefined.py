from .kphp_with_confdata import KphpWithConfdata


class TestConfdata(KphpWithConfdata):
    CONFDATA = {
        "a": "a_value",
        "a.a": "a_a_value",
        "ab": "ab_value",
        "ab.ab": "ab_ab_value",
        "abc": "abc_value",
        "abc.abc": "abc_abc_value",
        "some_key1": "some_value1",
        "some_key2": "some_value2",
        "one_dot.key1": "one_dot_value1",
        "one_dot.key2": "one_dot_value2",
        "two.dots.key1": "two_dots_value1",
        "two.dots.key2": "two_dots_value2",
        "three.dots.key.1": "three_dots_value_2",
        "three.dots.key.2": "three_dots_value_1",
        "prefix_1.key1": "prefix_1_value1",
        "prefix_1_key2": "prefix_1_value2",
        "prefix_1key3": "prefix_1value3",
        "prefix_a.key1": "prefix_a_value1",
        "prefix_ab_key1": "prefix_ab_value1",
        "prefix_abc_key1": "prefix_abc_value1",
        "prefix.xyz.1": "prefix_xyz_value1",
        "prefix.xyz.2": "prefix_xyz_value2",
        "prefix.xyz.3": "prefix_xyz_value3",
        "prefix.foo.key1": "prefix_foo_value1",
        "prefix.foo.key2": "prefix_foo_value2",
        "prefix.bar.key": "prefix_bar_value"
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
                "elements_simple_key": 2,
                "elements_one_dot_wildcard": 17,
                "elements_two_dots_wildcard": 10,
                "elements_predefined_wildcard": 15,
                "elements_with_delay": 0,
                "wildcards_one_dot": 9,
                "wildcards_two_dots": 5,
                "wildcards_predefined": 6,
                "vars_in_garbage_last": self.cmpGe(0),
                "vars_in_garbage_last_100_max": self.cmpGe(0),
                "vars_in_garbage_last_100_avg": self.cmpGe(0),
                "binlog_events_set_forever_total": len(self.CONFDATA),
            })

    def test_get_value(self):
        for key, value in self.CONFDATA.items():
            self.assertEqual(self._call_function("get_value", key), value)

        for key in ("unkown_key", "some_key", "one_dot.", "prefix_1", "a.", "prefix."):
            self.assertIsNone(self._call_function("get_value", key))

    def _do_test_for_predefined(self, function):
        for key in ("non_existed.", "non.existed.", ".", ".."):
            r = self._call_function(function, key)
            self.assertEqual(r, [])

        r = self._call_function(function, "a")
        self.assertEqual(r, {
            "": "a_value",
            ".a": "a_a_value",
            "b": "ab_value",
            "b.ab": "ab_ab_value",
            "bc": "abc_value",
            "bc.abc": "abc_abc_value",
        })

        r = self._call_function(function, "a.")
        self.assertEqual(r, {"a": "a_a_value"})

        r = self._call_function(function, "ab.")
        self.assertEqual(r, {"ab": "ab_ab_value"})

        r = self._call_function(function, "abc")
        self.assertEqual(r, {
            "": "abc_value",
            ".abc": "abc_abc_value",
        })

        r = self._call_function(function, "abc.")
        self.assertEqual(r, {"abc": "abc_abc_value"})

        r = self._call_function(function, "abc.")
        self.assertEqual(r, {"abc": "abc_abc_value"})

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

        r = self._call_function(function, "prefix_1")
        self.assertEqual(r, {
            ".key1": "prefix_1_value1",
            "_key2": "prefix_1_value2",
            "key3": "prefix_1value3",
        })

        r = self._call_function(function, "prefix_1.")
        self.assertEqual(r, {"key1": "prefix_1_value1"})

        r = self._call_function(function, "prefix_a")
        self.assertEqual(r, {
            ".key1": "prefix_a_value1",
            "b_key1": "prefix_ab_value1",
            "bc_key1": "prefix_abc_value1",
        })

        r = self._call_function(function, "prefix_a.")
        self.assertEqual(r, {"key1": "prefix_a_value1"})

        r = self._call_function(function, "prefix_abc")
        self.assertEqual(r, {"_key1": "prefix_abc_value1"})

        r = self._call_function(function, "prefix.xyz")
        self.assertEqual(r, {
            ".1": "prefix_xyz_value1",
            ".2": "prefix_xyz_value2",
            ".3": "prefix_xyz_value3",
        })

        r = self._call_function(function, "prefix.xyz.")
        self.assertEqual(r, {
            "1": "prefix_xyz_value1",
            "2": "prefix_xyz_value2",
            "3": "prefix_xyz_value3"
        })

        r = self._call_function(function, "prefix.")
        self.assertEqual(r, {
            "xyz.1": "prefix_xyz_value1",
            "xyz.2": "prefix_xyz_value2",
            "xyz.3": "prefix_xyz_value3",
            "foo.key1": "prefix_foo_value1",
            "foo.key2": "prefix_foo_value2",
            "bar.key": "prefix_bar_value"
        })

        r = self._call_function(function, "prefix.foo.")
        self.assertEqual(r, {
            "key1": "prefix_foo_value1",
            "key2": "prefix_foo_value2"
        })

        r = self._call_function(function, "prefix.bar.")
        self.assertEqual(r, {"key": "prefix_bar_value"})

    def test_get_values_by_predefined_wildcard(self):
        self._do_test_for_predefined("get_values_by_predefined_wildcard")

    def test_get_values_by_any_wildcard(self):
        self._do_test_for_predefined("get_values_by_any_wildcard")

        r = self._call_function("get_values_by_any_wildcard", "ab")
        self.assertEqual(r, {
            "": "ab_value",
            ".ab": "ab_ab_value",
            "c": "abc_value",
            "c.abc": "abc_abc_value",
        })

        r = self._call_function("get_values_by_any_wildcard", "prefix")
        self.assertEqual(r, {
            "_1.key1": "prefix_1_value1",
            "_1_key2": "prefix_1_value2",
            "_1key3": "prefix_1value3",
            "_a.key1": "prefix_a_value1",
            "_ab_key1": "prefix_ab_value1",
            "_abc_key1": "prefix_abc_value1",
            ".xyz.1": "prefix_xyz_value1",
            ".xyz.2": "prefix_xyz_value2",
            ".xyz.3": "prefix_xyz_value3",
            ".foo.key1": "prefix_foo_value1",
            ".foo.key2": "prefix_foo_value2",
            ".bar.key": "prefix_bar_value"
        })

        r = self._call_function("get_values_by_any_wildcard", "prefix.x")
        self.assertEqual(r, {
            "yz.1": "prefix_xyz_value1",
            "yz.2": "prefix_xyz_value2",
            "yz.3": "prefix_xyz_value3"
        })

        r = self._call_function("get_values_by_any_wildcard", "prefix_ab")
        self.assertEqual(r, {
            "_key1": "prefix_ab_value1",
            "c_key1": "prefix_abc_value1"
        })

    def test_get_values_by_predefined_wildcard_warning(self):
        for key in ("foo", "hello.world", "...", "ab", "prefix.x", "prefix"):
            r = self._call_function("get_values_by_predefined_wildcard", key)
            self.assertEqual(r, [])
            self.kphp_server.assert_log(["Warning: Trying to get elements by non predefined wildcard '{}'".format(key)])
