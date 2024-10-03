import socket

from python.lib.testcase import KphpServerAutoTestCase
from python.lib.http_client import RawResponse


class TestHeaderRegisterCallback(KphpServerAutoTestCase):
    def test_rpc_in_callback(self):
        self.kphp_server.http_request(uri='/test_header_register_callback?act_in_callback=rpc&port={}'.format(str(self.kphp_server.master_port)), timeout=5)
        self.kphp_server.assert_log(["test_header_register_callback/rpc_in_callback"], timeout=5)

    def test_exit_in_callback(self):
        response = self.kphp_server.http_request(uri='/test_header_register_callback?act_in_callback=exit&port={}'.format(str(self.kphp_server.master_port)), timeout=5)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"test_header_register_callback/exit_in_callback")