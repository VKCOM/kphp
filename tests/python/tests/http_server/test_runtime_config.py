from python.lib.testcase import WebServerAutoTestCase


class TestRuntimeConfig(WebServerAutoTestCase):

    @classmethod
    def extra_class_setup(cls):
        if not cls.should_use_k2():
            # For K2 case runtime-config is declared in component-config.yaml
            cls.web_server.update_options({
                "--runtime-config": "./data/runtime_configuration.json"
            })

    def test_read_only(self):
        response = self.web_server.http_request(uri="/test_runtime_config?mode=read_only")
        self.assertEqual(200, response.status_code)
        self.assertEqual(response.reason, "OK")
        self.assertEqual(response.content, b'name : simple configuration version : 1.0.0 OK')

    def test_modifying(self):
        response = self.web_server.http_request(uri="/test_runtime_config?mode=modify")
        self.assertEqual(200, response.status_code)
        self.assertEqual(response.reason, "OK")
        self.assertEqual(response.content, b'modified version : 1.0.1 old version : 1.0.0 OK')
