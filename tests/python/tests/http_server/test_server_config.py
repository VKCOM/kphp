import typing
import pytest

from python.lib.testcase import WebServerAutoTestCase
from python.lib.kphp_server import KphpServer


@pytest.mark.k2_skip_suite
class TestServerConfig(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--server-config": "data/server-config.yml"
        })

    def test_cluster_name_from_config(self):
        resp = self.web_server.http_request("/server-status", http_port=typing.cast(KphpServer, self.web_server).master_port)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.content.decode("utf-8").split('\n')[0].split("\t")[1], "custom_cluster_name")
