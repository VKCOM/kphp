import pytest
from python.lib.testcase import WebServerAutoTestCase

class TestStoreFetchDeleteSerialized(WebServerAutoTestCase):
    def test_store_fetch_delete_serialized(self):
        elements = 300

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
            self.assertEqual(resp.json(), {"is_valid": True})
            
            resp = self.web_server.http_post(
                uri="/delete",
                json={"key": "key{}".format(i)})
            self.assertEqual(resp.status_code, 200)
