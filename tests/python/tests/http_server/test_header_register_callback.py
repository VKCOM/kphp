import typing
import pytest

from python.lib.kphp_server import KphpServer
from python.lib.testcase import WebServerAutoTestCase


class TestHeaderRegisterCallback(WebServerAutoTestCase):

    @pytest.mark.k2_skip
    def test_rpc_in_callback(self):
        self.web_server.http_request(uri='/test_header_register_callback?act_in_callback=rpc&port={}'.format(str(typing.cast(KphpServer, self.web_server).master_port)), timeout=5)
        self.web_server.assert_log(["test_header_register_callback/rpc_in_callback"], timeout=5)

    def test_exit_in_callback(self):
        response = self.web_server.http_request(uri='/test_header_register_callback?act_in_callback=exit', timeout=5)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"test_header_register_callback/exit_in_callback")
