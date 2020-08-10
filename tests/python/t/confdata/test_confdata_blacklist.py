from .kphp_with_confdata import KphpWithConfdata


class TestConfdataBlacklist(KphpWithConfdata):
    CONFDATA = {
        "some_key1": "some_value1",
        "some_key2": "some_value2",
        "blacklisted.key1": "blacklisted_value1",
        "blacklisted.key2": "blacklisted_value2"
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
                "elements_total": len(self.CONFDATA) - 2,
                "elements_simple_key": 2,
                "elements_one_dot_wildcard": 0,
                "elements_two_dots_wildcard": 0,
                "elements_predefined_wildcard": 0,
                "elements_with_delay": 0,
                "wildcards_one_dot": 0,
                "wildcards_two_dots": 0,
                "wildcards_predefined": 0,
                "vars_in_garbage_last": self.cmpGe(0),
                "vars_in_garbage_last_100_max": self.cmpGe(0),
                "vars_in_garbage_last_100_avg": self.cmpGe(0),
                "binlog_events_set_forever_total": len(self.CONFDATA),
                "binlog_events_set_forever_blacklisted": 2
            })

    def test_get_blacklisted_value(self):
        for key in ("blacklisted.key1", "blacklisted.key2", "blacklisted.key3", "blacklisted.key4"):
            self.assertIsNone(self._call_function("get_value", key))
            self.kphp_server.assert_log(["Warning: Trying to get blacklisted key '{}'".format(key)])

    def test_get_blacklisted_wildcard(self):
        self.assertEqual(self._call_function("get_values_by_any_wildcard", "blacklisted."), [])
        self.assertEqual(self._call_function("get_values_by_predefined_wildcard", "blacklisted."), [])
