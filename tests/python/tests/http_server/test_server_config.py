from python.lib.testcase import KphpServerAutoTestCase


class TestServerConfig(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--server-config": "data/server-config.yml"
        })

    def test_cluster_name_from_config(self):
        resp = self.kphp_server.http_request("/server-status", http_port=self.kphp_server.master_port)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.content.decode("utf-8").split('\n')[0].split("\t")[1], "custom_cluster_name")
