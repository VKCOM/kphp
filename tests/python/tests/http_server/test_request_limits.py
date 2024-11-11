import random
import string

from python.lib.testcase import KphpServerAutoTestCase


class TestRequestLimits(KphpServerAutoTestCase):
    @staticmethod
    def _get_rand_str(str_len):
        return ''.join(random.choice(string.ascii_letters) for _ in range(str_len))

    def test_query_limit(self):
        uri = "/test_limits_" + self._get_rand_str(185)
        resp = self.kphp_server.http_get(uri)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["REQUEST_URI"], uri)

        uri = "/test_limits?" + self._get_rand_str(15000)
        resp = self.kphp_server.http_get(uri)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["REQUEST_URI"], uri)

        # Base on MAX_HTTP_HEADER_QUERY_WORD_SIZE 16 * 1024
        resp = self.kphp_server.http_get(uri="/{}".format(("a" * (16 * 1024))))
        self.assertEqual(resp.status_code, 414)

        resp = self.kphp_server.http_get(uri="/a?{}".format("a" * 17000))
        self.assertEqual(resp.status_code, 414)

    def test_headers_limit(self):
        key = self._get_rand_str(4000)
        resp = self.kphp_server.http_get("/test_limits", headers={key: "y"})
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["HTTP_" + key.upper()], "y")

        resp = self.kphp_server.http_get(headers={"x" * 5000: "y"})
        self.assertEqual(resp.status_code, 400)

        value = self._get_rand_str(17000)
        resp = self.kphp_server.http_get("/test_limits", headers={"x": value})
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["HTTP_X"], value)

        resp = self.kphp_server.http_get(headers={"x": "y" * 70000})
        self.assertEqual(resp.status_code, 431)

        resp = self.kphp_server.http_get(headers={"x" * 5000: "y" * 70000})
        self.assertEqual(resp.status_code, 400)

        uri = "/test_limits?" + self._get_rand_str(15000)
        value = self._get_rand_str(15000)
        resp = self.kphp_server.http_get(uri, headers={key: value})
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["HTTP_" + key.upper()], value)
        self.assertEqual(resp.json()["REQUEST_URI"], uri)

        key2 = self._get_rand_str(4000)
        value2 = self._get_rand_str(15000)
        resp = self.kphp_server.http_get(uri, headers={key: value, key2: value2})
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["HTTP_" + key.upper()], value)
        self.assertEqual(resp.json()["HTTP_" + key2.upper()], value2)
        self.assertEqual(resp.json()["REQUEST_URI"], uri)

        resp = self.kphp_server.http_get(
            uri="/a?{}".format("a" * 15000),
            headers={
                "x" * 4000: "a" * 15000,
                "y" * 4000: "b" * 15000,
                "z" * 4000: "c" * 15000,
            })
        self.assertEqual(resp.status_code, 431)

    def test_post_limit(self):
        req_size = 4000
        resp = self.kphp_server.http_post("/test_big_post_data", data="x" * req_size)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["len"], req_size)

        req_size = (2 * 1024 * 1024 - 1)
        resp = self.kphp_server.http_post("/test_big_post_data", data="x" * req_size)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["len"], (2 * 1024 * 1024 - 1))

        resp = self.kphp_server.http_post("/test_big_post_data", data="x" * (2 * 1024 * 1024 + 1))
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json()["len"], 0)

