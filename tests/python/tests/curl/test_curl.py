from python.lib.testcase import KphpServerAutoTestCase


class TestCurl(KphpServerAutoTestCase):
    maxDiff = 16 * 1024

    def _curl_request(self, uri, return_transfer=1, post=None, headers=None):
        resp = self.kphp_server.http_post(
            uri="/test_curl",
            json={
                "url": "localhost:{}{}".format(self.kphp_server.http_port, uri),
                "return_transfer": return_transfer,
                "post": post,
                "headers": headers
            })
        self.assertEqual(resp.status_code, 200)
        return resp.json()

    def _prepare_result(self, uri, method, extra=None):
        res = {
            "HTTP_ACCEPT": "*/*",
            "HTTP_HOST": "localhost:{}".format(self.kphp_server.http_port),
            "REQUEST_METHOD": method,
            "REQUEST_URI": uri,
            "SERVER_PROTOCOL": "HTTP/1.1"
        }
        if extra:
            res.update(extra)
        return res

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
