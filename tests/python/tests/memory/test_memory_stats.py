import pytest

from python.lib.testcase import WebServerAutoTestCase


class TestMemoryUsage(WebServerAutoTestCase):
    def _template(self, test_case: str, expected_usage: int, expected_usage_after_unset: int, expected_peak_usage: int):
        response = self.web_server.http_post(f"/{test_case}")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(
            response.json(),
            {
                "usage": expected_usage,
                "usage_after_unset": expected_usage_after_unset,
                "peak_usage": expected_peak_usage,
            },
        )

    def test_memory_usage(self):
        self._template(
            "test_memory_usage", expected_usage=2**20, expected_usage_after_unset=0, expected_peak_usage=2**19 + 2**20
        )

    @pytest.mark.k2_skip
    def test_total_memory_usage_kphp(self):
        self._template(
            "test_total_memory_usage",
            expected_usage=48 + sum(2**n for n in range(12, 21)),
            expected_usage_after_unset=48 + sum(2**n for n in range(12, 20)),
            expected_peak_usage=48 + sum(2**n for n in range(12, 21)),
        )

    @pytest.mark.kphp_skip
    def test_total_memory_usage_k2(self):
        self._template(
            "test_total_memory_usage",
            expected_usage=96 + sum(2**n for n in range(12, 21)),
            expected_usage_after_unset=96 + sum(2**n for n in range(12, 20)),
            expected_peak_usage=96 + sum(2**n for n in range(12, 21)),
        )
