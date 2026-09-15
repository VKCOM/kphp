import pytest
from python.lib.testcase import WebServerAutoTestCase

@pytest.mark.k2_skip_suite
class TestStoreFetchDelete(WebServerAutoTestCase):
    def test_store_fetch_delete(self):
        elements = 300

        stats_before = self.web_server.get_stats(prefix="kphp_server.instance_cache_")
        for i in range(elements):
            resp = self.web_server.http_post(
                uri="/store",
                json={"key": "key{}".format(i)})
            self.assertEqual(resp.status_code, 200)
            self.assertEqual(resp.json(), {"result": True})

            resp = self.web_server.http_post(
                uri="/fetch_and_verify",
                json={"key": "key{}".format(i)})
            self.assertEqual(resp.status_code, 200)
            self.assertEqual(resp.json(), {"a": True, "b": True, "c": True})
            
            resp = self.web_server.http_post(
                uri="/delete",
                json={"key": "key{}".format(i)})
            self.assertEqual(resp.status_code, 200)

        self.web_server.assert_stats(
            timeout=70,
            initial_stats=stats_before,
            prefix="kphp_server.instance_cache_",
            expected_added_stats={
                "memory_used": 0,
                "elements_stored": elements,
                "elements_destroyed": elements
            })
