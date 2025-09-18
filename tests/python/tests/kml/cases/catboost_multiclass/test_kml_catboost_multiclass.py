from python.lib.testcase import WebServerAutoTestCase

class TestKmlCatboostMulticlass(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--kml-dir": cls.test_dir + "/../../models",
        })
    def test_store_fetch_delete_serialized(self):
        resp = self.web_server.http_get(uri="/")
        self.assertEqual(200, resp.status_code)
