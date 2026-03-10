import os
import pytest

from python.lib.testcase import WebServerAutoTestCase

class TestClusterName(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if cls.should_use_k2():
            os.environ["CLUSTER_NAME"] = "custom_cluster_name"
        else:
            cls.web_server.update_options({
                "--server-config": "data/server-config.yml"
            })

    @classmethod
    def extra_class_teardown(cls):
        if cls.should_use_k2():
            os.environ.pop("CLUSTER_NAME")

    def test_get_cluster_name(self):
        resp = self.web_server.http_request(uri="/test_get_cluster_name")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.content.decode("utf-8"), "custom_cluster_name")
