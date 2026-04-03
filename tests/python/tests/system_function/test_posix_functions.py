import json
import os
import pwd
import typing
from python.lib.testcase import WebServerAutoTestCase
from python.lib.kphp_server import KphpServer

class TestErrors(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if not cls.should_use_k2():
            cls.web_server.update_options({
                "--workers-num": 1
            })

    def test_posix_getpid(self):
        pid = 0
        if isinstance(self.web_server, KphpServer):
            pid = typing.cast(KphpServer, self.web_server).get_workers()[0].pid
        else:
            pid = self.web_server.pid

        response = self.web_server.http_request(uri="/test_posix_getpid", method='GET')
        self.assertEqual(pid, json.loads(response.text)["pid"])
        self.assertEqual(200, response.status_code)

    def test_posix_getuid(self):
        response = self.web_server.http_request(uri="/test_posix_getuid", method='GET')
        self.assertEqual(os.getuid(), json.loads(response.text)["uid"])
        self.assertEqual(200, response.status_code)

    def test_posix_getpwuid(self):
        response = self.web_server.http_request(uri="/test_posix_getpwuid", method='GET')
        expected = pwd.getpwuid(os.getuid())
        got = json.loads(response.text)
        self.assertEqual(expected[0], got['name'])
        self.assertEqual(expected[1], got['passwd'])
        self.assertEqual(expected[2], got['uid'])
        self.assertEqual(expected[3], got['gid'])
        self.assertEqual(expected[4], got['gecos'])
        self.assertEqual(expected[5], got['dir'])
        self.assertEqual(expected[6], got['shell'])
        self.assertEqual(200, response.status_code)
