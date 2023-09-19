from python.tests.curl.curl_test_case import CurlTestCase


class TestCurl(CurlTestCase):
    test_case_uri="/test_curl"

    def test_curl_get_return_transfer(self):
        self.assertEqual(self._curl_request("/echo/test_get"), {
            "exec_result": self._prepare_result("/echo/test_get", "GET")
        })

    def test_curl_get_no_return_transfer(self):
        self.assertEqual(self._curl_request("/echo/test_get", return_transfer=0), {
            "exec_result": True,
            "output_buffer": self._prepare_result("/echo/test_get", "GET")
        })

    def test_curl_post_return_transfer(self):
        self.assertEqual(self._curl_request("/echo/test_post", post={"hello": "world!"}), {
            "exec_result": self._prepare_result("/echo/test_post", "POST", {"POST_BODY": {"hello": "world!"}})
        })

    def test_curl_post_no_return_transfer(self):
        self.assertEqual(self._curl_request("/echo/test_post", return_transfer=0, post={"hello": "world!"}), {
            "exec_result": True,
            "output_buffer": self._prepare_result("/echo/test_post", "POST", {"POST_BODY": {"hello": "world!"}})
        })

    def test_curl_out_headers(self):
        self.assertEqual(
            self._curl_request("/echo/test_headers", headers=["hello: world", "foo: bar"]), {
                "exec_result": self._prepare_result("/echo/test_headers", "GET", extra={
                    "HTTP_HELLO": "world",
                    "HTTP_FOO": "bar"
                })})

    def test_curl_timeout(self):
        self.assertEqual(self._curl_request("/echo/test_get", timeout=0.1), {
            "exec_result": False
        })

    def test_curl_nonexistent_url(self):
        self.assertEqual(self._curl_request("nonexistent_url"), {
            "exec_result": False
        })

    def test_curl_connection_only_success(self):
        self.assertEqual(self._curl_request("/echo/test_get", connect_only=True), {
            "exec_result": ''
        })

    def test_curl_connection_only_fail(self):
        self.assertEqual(self._curl_request("nonexistent_url", connect_only=True), {
            "exec_result": False
        })
