import pytest
from python.lib.testcase import WebServerAutoTestCase


class TestMethod(WebServerAutoTestCase):
    def test_method_get(self):
        response = self.web_server.http_request(uri="/", method='GET')
        self.assertEqual(200, response.status_code)
        self.assertEqual("OK", response.reason)

    def test_method_post(self):
        response = self.web_server.http_request(uri="/", method='POST')
        self.assertEqual(200, response.status_code)
        self.assertEqual("OK", response.reason)

    def test_method_head(self):
        response = self.web_server.http_request(uri="/", method='HEAD')
        self.assertEqual(200, response.status_code)
        self.assertEqual("OK", response.reason)

    @pytest.mark.k2_skip
    def test_method_put(self):
        response = self.web_server.http_request(uri="/", method='PUT')
        self.assertEqual(400, response.status_code)
        self.assertEqual("Bad Request", response.reason)

    @pytest.mark.k2_skip
    def test_method_delete(self):
        response = self.web_server.http_request(uri="/", method='DELETE')
        self.assertEqual(400, response.status_code)
        self.assertEqual("Bad Request", response.reason)

    @pytest.mark.k2_skip
    def test_method_patch(self):
        response = self.web_server.http_request(uri="/", method='PATCH')
        self.assertEqual(400, response.status_code)
        self.assertEqual("Bad Request", response.reason)

    @pytest.mark.k2_skip
    def test_method_trace(self):
        response = self.web_server.http_request(uri="/", method='TRACE')
        self.assertEqual(400, response.status_code)
        self.assertEqual("Bad Request", response.reason)

    @pytest.mark.k2_skip
    def test_method_foobarbaz(self):
        response = self.web_server.http_request(uri="/", method='FOOBARBAZ')
        self.assertEqual(400, response.status_code)
        self.assertEqual("Bad Request", response.reason)

    @pytest.mark.k2_skip
    def test_method_options(self):
        response = self.web_server.http_request(uri="/", method='OPTIONS')
        self.assertEqual(501, response.status_code)
        self.assertEqual("Not Implemented", response.reason)
