from python.lib.testcase import WebServerAutoTestCase


class TestMemoryUsage(WebServerAutoTestCase):
    LOG_PAGE_SIZE = 12                      # log_2(4096)
    LOG_EXPECTED_LAST_ALLOCATION_SIZE = 20  # ceil(log_2(1e6))

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
            expected_usage=2**self.LOG_EXPECTED_LAST_ALLOCATION_SIZE,
            expected_usage_after_cleaning=0,
            expected_peak_usage=2**self.LOG_EXPECTED_LAST_ALLOCATION_SIZE + # last allocation size
            2**(self.LOG_EXPECTED_LAST_ALLOCATION_SIZE - 1)                 # prev allocation size
        )

    def test_total_memory_usage(self):
        self._template(
            "test_total_memory_usage",
            expected_usage=sum(
                2**n for n in range(self.LOG_PAGE_SIZE + 1, self.LOG_EXPECTED_LAST_ALLOCATION_SIZE + 1)
            ),
            expected_usage_after_cleaning=sum(
                2**n for n in range(self.LOG_PAGE_SIZE + 1, self.LOG_EXPECTED_LAST_ALLOCATION_SIZE)
            ),
            expected_peak_usage=sum(
                2**n for n in range(self.LOG_PAGE_SIZE + 1, self.LOG_EXPECTED_LAST_ALLOCATION_SIZE + 1)
            ),
        )
