import socket
import pytest

from python.lib.testcase import WebServerAutoTestCase
from python.lib.http_client import RawResponse

class TestErrors(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if not cls.should_use_k2():
            cls.web_server.update_options({
                "--workers-num": 1
            })

    def test_get_running_fork_id(self):
        response = self.web_server.http_request(uri="/test_get_running_fork_id", method='GET')
        self.assertEqual(200, response.status_code)
