from python.lib.testcase import KphpServerAutoTestCase


class CurlTestCase(KphpServerAutoTestCase):
    maxDiff = 16 * 1024

    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 2,
        })

    def _curl_request(self, uri, return_transfer=1, post=None, headers=None, timeout=None, connect_only=None):
        url = "localhost:{}{}".format(self.kphp_server.http_port, uri) if uri.startswith('/') else uri
        resp = self.kphp_server.http_post(
            uri=self.test_case_uri,
            json={
                "url": url,
                "return_transfer": return_transfer,
                "post": post,
                "headers": headers,
                "timeout": timeout,
                "connect_only": connect_only
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
