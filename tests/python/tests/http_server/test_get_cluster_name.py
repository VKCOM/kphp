from python.lib.testcase import WebServerAutoTestCase

class TestClusterName(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if not cls.should_use_k2():
            cls.web_server.update_options({
                "--server-config": "data/server-config.yml"
            })

    def test_get_cluster_name(self):
        resp = self.web_server.http_request(uri="/test_get_cluster_name")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.content.decode("utf-8"), "custom_cluster_name")
