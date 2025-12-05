from python.lib.testcase import WebServerAutoTestCase

class CurlTestCase(WebServerAutoTestCase):
    maxDiff = 16 * 1024

    @classmethod
    def extra_class_setup(cls):
        if cls.should_use_k2():
            cls.web_server.ignore_log_errors()
        else:
            cls.web_server.update_options({
                "--workers-num": 2,
            })

    def _curl_request(self, uri, return_transfer=1, post=None, headers=None, timeout=None, connect_only=None):
        url = "localhost:{}{}".format(self.web_server.http_port, uri) if uri.startswith('/') else uri
        resp = self.web_server.http_post(
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
            "HTTP_HOST": "localhost:{}".format(self.web_server.http_port),
            "REQUEST_METHOD": method,
            "REQUEST_URI": uri,
            "SERVER_PROTOCOL": "HTTP/1.1"
        }
        if extra:
            res.update(extra)
        return res

class CurlMultiTestCase(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if cls.should_use_k2():
            cls.web_server.ignore_log_errors()
        else:
            cls.web_server.update_options({
                "--workers-num": 3,
            })

    def _curl_multi_request(self, uri1, uri2):
        url1 = "localhost:{}{}".format(self.web_server.http_port, uri1) if uri1.startswith('/') else uri1
        url2 = "localhost:{}{}".format(self.web_server.http_port, uri2) if uri2.startswith('/') else uri2
        resp = self.web_server.http_post(
            uri=self.test_case_uri,
            json={
                "url1": url1,
                "url2": url2,
            })
        self.assertEqual(resp.status_code, 200)
        return resp.json()
