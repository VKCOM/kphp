import time
import urllib
import socket

from python.lib.testcase import KphpServerAutoTestCase


class TestIgnoreUserAbort(KphpServerAutoTestCase):
    def _send_request(self, uri="/", timeout=0.05):
        try:
            self.kphp_server.http_request(uri=uri, timeout=timeout)
        except Exception:
            pass

    def test_user_abort_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=no_ignore')
        self.kphp_server.assert_log(['Critical error during script execution: http connection close'], timeout=5)
        error = False
        try:
            self.kphp_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "no_ignore"], timeout=2)
        except Exception:
            error = True
        self.assertTrue(error)

    def test_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=no_ignore')
        self.kphp_server.assert_log(['Critical error during script execution: http connection close'], timeout=5)
        error = False
        try:
            self.kphp_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "no_ignore"], timeout=2)
        except Exception:
            error = True
        self.assertTrue(error)

    def test_ignore_user_abort_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=ignore&port={}'.format(str(self.kphp_server.master_port)))
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_ignore_" + "rpc"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_ignore_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=ignore')
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_ignore_" + "resumable"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_nested_ignore_user_abort_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=nested_ignore&port={}'.format(str(self.kphp_server.master_port)))
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "nested_ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_nested_ignore_" + "rpc"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_nested_ignore_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=nested_ignore')
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "nested_ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_nested_ignore_" + "resumable"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_multi_ignore_user_abort_rpc_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=multi_ignore&port={}'.format(str(self.kphp_server.master_port)))
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "multi_ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_multi_ignore_" + "rpc"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_multi_ignore_user_abort_resumable_work(self):
        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=multi_ignore')
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "multi_ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_multi_ignore_" + "resumable"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)

    def test_idempotence_ignore_user_abort(self):
        self._send_request(uri='/test_ignore_user_abort?type=rpc&level=ignore&port={}'.format(str(self.kphp_server.master_port)))
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_rpc_work_" + "ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_ignore_" + "rpc"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)

        time.sleep(2)

        self._send_request(uri='/test_ignore_user_abort?type=resumable&level=nested_ignore')
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_resumable_work_" + "nested_ignore"], timeout=5)
        self.kphp_server.assert_log(["test_ignore_user_abort/finish_nested_ignore_" + "resumable"], timeout=5)
        self.kphp_server.assert_log(["shutdown_function was called"], timeout=5)
