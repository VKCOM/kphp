import pytest

from python.lib.kphp_server import KphpServer
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestHeaderRegisterCallback(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.rpc_test_port = 0
        if isinstance(cls.web_server, KphpServer):
            cls.rpc_test_port = cls.web_server.master_port

    def test_rpc_in_callback(self):
        self.web_server.http_request(uri='/test_header_register_callback?act_in_callback=rpc&port={}'.format(str(self.rpc_test_port)), timeout=5)
        self.web_server.assert_log(["test_header_register_callback/rpc_in_callback"], timeout=5)

    def test_exit_in_callback(self):
        response = self.web_server.http_request(uri='/test_header_register_callback?act_in_callback=exit&port={}'.format(str(self.rpc_test_port)), timeout=5)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"test_header_register_callback/exit_in_callback")