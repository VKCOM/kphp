import json
import os
from python.lib.testcase import WebServerAutoTestCase

directory_path = os.path.join(os.path.dirname(__file__), "data")
prepared_suffix = "_prepared"


class TestPrepareSearchQuery(WebServerAutoTestCase):
    def test_prepare_search_query(self):
        for file in os.listdir(directory_path):
            if not os.path.basename(file).endswith(prepared_suffix):
                with open(os.path.join(directory_path, file), "r") as query_file:
                    with open(os.path.join(directory_path, file + prepared_suffix), "r") as prepared_query_file:
                        query = query_file.read()
                        expected_prepared_query = prepared_query_file.read()
                        if len(expected_prepared_query) > 0 and expected_prepared_query[-1] == '\n':
                            expected_prepared_query = expected_prepared_query[:-1]

                        headers = {"Content-Type": "text/plain; charset=utf-8"}
                        resp = self.web_server.http_post(headers=headers, data=query.encode("utf-8"))

                        self.assertEqual(resp.status_code, 200)
                        result = json.loads(resp.text)["POST_BODY"]
                        self.assertEqual(result, expected_prepared_query)
