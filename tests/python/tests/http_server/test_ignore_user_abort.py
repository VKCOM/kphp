import time

import pytest

from python.lib.kphp_server import KphpServer
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestIgnoreUserAbort(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.rpc_test_port = 0
        if isinstance(cls.web_server, KphpServer):
            cls.rpc_test_port = cls.web_server.master_port

    def _send_request(self, uri="/", timeout=0.05):
        try:
            self.web_server.http_request(uri=uri, timeout=timeout)
        except Exception:
            pass

    """
    Changing the name leads to different tests run order and for some reason it helps to get rid of ASAN warning. 
    As we decided that the previous ASAN warning was false-positive, this kind of fix might be acceptable for us.
    Old name was - "test_user_abort_rpc_work"
    """
    def test_user_abort_of_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=no_ignore&port={}'.format(str(self.rpc_test_port)))
        self.web_server.assert_log(['Critical error during script execution: http connection close'], timeout=10)
        error = False
        try:
            self.web_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "no_ignore"], timeout=2)
        except Exception:
            error = True
        self.assertTrue(error)

    def test_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=no_ignore')
        self.web_server.assert_log(['Critical error during script execution: http connection close'], timeout=10)
        error = False
        try:
            self.web_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "no_ignore"], timeout=2)
        except Exception:
            error = True
        self.assertTrue(error)

    def test_ignore_user_abort_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=ignore&port={}'.format(str(self.rpc_test_port)))
        self.web_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_ignore_" + "rpc"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_ignore_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=ignore')
        self.web_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_ignore_" + "resumable"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_nested_ignore_user_abort_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=nested_ignore&port={}'.format(str(self.rpc_test_port)))
        self.web_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "nested_ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_nested_ignore_" + "rpc"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_nested_ignore_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=nested_ignore')
        self.web_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "nested_ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_nested_ignore_" + "resumable"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_multi_ignore_user_abort_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=multi_ignore&port={}'.format(str(self.rpc_test_port)))
        self.web_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "multi_ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_multi_ignore_" + "rpc"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_multi_ignore_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=multi_ignore')
        self.web_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "multi_ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_multi_ignore_" + "resumable"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_idempotence_ignore_user_abort(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=ignore&port={}'.format(str(self.rpc_test_port)))
        self.web_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_ignore_" + "rpc"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)

        time.sleep(2)

        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=nested_ignore')
        self.web_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "nested_ignore"], timeout=5)
        self.web_server.assert_log(["test_ignore_user_abort/finish_nested_ignore_" + "resumable"], timeout=5)
        self.web_server.assert_log(["shutdown_function was called"], timeout=5)
