from python.lib.testcase import WebServerAutoTestCase


class TestMemoryUsage(WebServerAutoTestCase):
    STRING_ALIGNMENT = 4096
    MAX_ALLOCATION_SIZE = 2**20   # 1 MB

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
            expected_usage=self.MAX_ALLOCATION_SIZE,
            expected_usage_after_cleaning=0,
            expected_peak_usage=self.MAX_ALLOCATION_SIZE +    # last allocation size
            self.MAX_ALLOCATION_SIZE // 2                     # prev allocation size
        )

    def test_total_memory_usage(self):
        self._template(
            "test_total_memory_usage",
            expected_usage=2 * self.MAX_ALLOCATION_SIZE - 2 * self.STRING_ALIGNMENT,              # 8192 + 16384 + ... + 2**20
            expected_usage_after_cleaning=self.MAX_ALLOCATION_SIZE - 2 * self.STRING_ALIGNMENT,   # 8192 + 16384 + ... + 2**19
            expected_peak_usage=2 * self.MAX_ALLOCATION_SIZE - 2 * self.STRING_ALIGNMENT,         # 8192 + 16384 + ... + 2**20
        )
