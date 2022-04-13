from python.lib.testcase import KphpServerAutoTestCase


class TestStoreFetchDelete(KphpServerAutoTestCase):
    def test_store_fetch_delete(self):
        elements = 600

        stats_before = self.kphp_server.get_stats(prefix="kphp_server.instance_cache_")
        for i in range(elements):
            resp = self.kphp_server.http_post(
                uri="/store",
                json={"key": "key{}".format(i)})
            self.assertEqual(resp.status_code, 200)
            self.assertEqual(resp.json(), {"result": True})

            resp = self.kphp_server.http_post(
                uri="/fetch_and_verify",
                json={"key": "key{}".format(i)})
            self.assertEqual(resp.status_code, 200)
            self.assertEqual(resp.json(), {"a": True, "b": True, "c": True})
            
            resp = self.kphp_server.http_post(
                uri="/delete",
                json={"key": "key{}".format(i)})
            self.assertEqual(resp.status_code, 200)

            stored_elements = i + 1
            if stored_elements % 300 == 0:
                # todo this waits for 2 minutes, making this test too long; can we avoid this?
                self.kphp_server.assert_stats(
                    initial_stats=stats_before,
                    timeout=120,
                    prefix="kphp_server.instance_cache_",
                    expected_added_stats={
                        "elements_stored": stored_elements,
                        "elements_destroyed": stored_elements
                    })

        self.kphp_server.assert_stats(
            initial_stats=stats_before,
            prefix="kphp_server.instance_cache_",
            expected_added_stats={
                "memory_used": 0,
                "elements_stored": elements,
                "elements_destroyed": elements
            })
