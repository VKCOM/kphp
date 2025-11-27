import json
import os
from python.lib.testcase import WebServerAutoTestCase

directory_path = "kphp/tests/python/tests/prepare_search_query/data"
prepared_suffix = "_prepared"


class TestPrepareSearchQuery(WebServerAutoTestCase):
    def test_prepare_search_query(self):
        for file in os.listdir(directory_path):
            if not os.path.basename(file).endswith(prepared_suffix):
                with open(os.path.join(directory_path, file), "r") as query_file:
                    with open(os.path.join(directory_path, file + prepared_suffix), "r") as prepared_query_file:
                        query = query_file.read()
                        expected_prepared_query = prepared_query_file.read()

                        d = {"post": query}
                        resp = self.web_server.http_post(json=d)

                        self.assertEqual(resp.status_code, 200)
                        result = json.loads(resp.text)["POST_BODY"]
                        self.assertEqual(result, expected_prepared_query)
