from python.lib.testcase import WebServerAutoTestCase


class TestMemoryUsage(WebServerAutoTestCase):
    PAGE_SIZE = 4096
    EXPECTED_LAST_ALLOCATION_SIZE = 2**20   # 1 MB

    def _template(self, test_case: str, expected_usage: int, expected_usage_after_cleaning: int, expected_peak_usage: int):
        response = self.web_server.http_post(f"/{test_case}")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(
            response.json(),
            {
                "usage": expected_usage,
                "usage_after_cleaning": expected_usage_after_cleaning,
                "peak_usage": expected_peak_usage,
            },
        )

    def test_memory_usage(self):
        self._template(
            "test_memory_usage",
            expected_usage=self.EXPECTED_LAST_ALLOCATION_SIZE,
            expected_usage_after_cleaning=0,
            expected_peak_usage=self.EXPECTED_LAST_ALLOCATION_SIZE +    # last allocation size
            self.EXPECTED_LAST_ALLOCATION_SIZE // 2                     # prev allocation size
        )

    def test_total_memory_usage(self):
        self._template(
            "test_total_memory_usage",
            expected_usage=(self.EXPECTED_LAST_ALLOCATION_SIZE << 1) - (self.PAGE_SIZE << 1),           # 8192 + 16384 + ... + 2**20
            expected_usage_after_cleaning=self.EXPECTED_LAST_ALLOCATION_SIZE - (self.PAGE_SIZE << 1),   # 8192 + 16384 + ... + 2**19
            expected_peak_usage=(self.EXPECTED_LAST_ALLOCATION_SIZE << 1) - (self.PAGE_SIZE << 1),      # 8192 + 16384 + ... + 2**20
        )
