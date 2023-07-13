import re
from python.tests.curl.curl_test_case import CurlTestCase


class TestCurlResumable(CurlTestCase):
    test_case_uri="/test_curl_resumable"

    def _assert_resumable_logs(self):
        server_log = self.kphp_server.get_log()
        pattern = "start_resumable_function(.|\n)*start_curl_query(.|\n)*end_resumable_function(.|\n)*end_curl_query"
        if not re.search(pattern, ''.join(server_log)):
            raise RuntimeError("cannot find match for pattern \"" + pattern + "\n")


    def test_curl_resumable_get_return_transfer(self):
        self.assertEqual(self._curl_request("/echo/test_get"), {
            "exec_result": self._prepare_result("/echo/test_get", "GET")
        })
        self._assert_resumable_logs();

    def test_curl_resumable_get_no_return_transfer(self):
        # return_transfer is set to true forcefully inside curl_exec_concurrently()
        self.assertEqual(self._curl_request("/echo/test_get", return_transfer=0), {
            "exec_result": self._prepare_result("/echo/test_get", "GET")
        })
        self._assert_resumable_logs();

    def test_curl_resumable_post_return_transfer(self):
        self.assertEqual(self._curl_request("/echo/test_post", post={"hello": "world!"}), {
            "exec_result": self._prepare_result("/echo/test_post", "POST", {"POST_BODY": {"hello": "world!"}})
        })
        self._assert_resumable_logs();

    def test_curl_resumable_post_no_return_transfer(self):
        # return_transfer is set to true forcefully inside curl_exec_concurrently()
        self.assertEqual(self._curl_request("/echo/test_post", return_transfer=0, post={"hello": "world!"}), {
            "exec_result": self._prepare_result("/echo/test_post", "POST", {"POST_BODY": {"hello": "world!"}})
        })
        self._assert_resumable_logs();

    def test_curl_resumable_out_headers(self):
        self.assertEqual(
            self._curl_request("/echo/test_headers", headers=["hello: world", "foo: bar"]), {
                "exec_result": self._prepare_result("/echo/test_headers", "GET", extra={
                    "HTTP_HELLO": "world",
                    "HTTP_FOO": "bar"
                })})

    def test_curl_resumable_timeout(self):
        self.assertEqual(self._curl_request("/echo/test_get", timeout=0.1), {
            "exec_result": False
        })

    def test_curl_resumable_nonexistent_url(self):
        self.assertEqual(self._curl_request("nonexistent_url"), {
            "exec_result": False
        })

    def test_curl_resumable_connection_only_success(self):
        self.assertEqual(self._curl_request("/echo/test_get", connect_only=True), {
            "exec_result": ''
        })

    def test_curl_resumable_connection_only_fail(self):
        self.assertEqual(self._curl_request("nonexistent_url", connect_only=True), {
            "exec_result": False
        })
