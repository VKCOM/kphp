import os
from python.lib.testcase import WebServerAutoTestCase

directory_path = "data"
prepared_suffix = "_prepared"


class TestShutdownFunctions(WebServerAutoTestCase):
    def test_prepare_search_query(self):
        for file in os.listdir(directory_path):
            if not os.path.basename(file).endswith(prepared_suffix):
                with open(file, "r", encoding="utf-8") as query_file:
                    with open(file + prepared_suffix, "r", encoding="utf-8") as prepared_query_file:
                        query = query_file.read()
                        expected_prepared_query = prepared_query_file.read()

                        resp = self.web_server.http_post(query)

                        self.assertEqual(resp.status_code, 200)
                        self.assertEqual(resp.text, expected_prepared_query)
