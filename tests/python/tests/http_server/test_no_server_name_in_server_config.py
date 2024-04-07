from python.lib.testcase import KphpServerAutoTestCase
import socket


class TestNoServerNameInServerConfig(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({})

    def test_server_name_from_config(self):
        resp = self.kphp_server.http_request("/server-status", http_port=self.kphp_server.master_port)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.content.decode("utf-8").split('\n')[1].split("\t")[1], socket.gethostname())
