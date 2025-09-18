from python.lib.testcase import WebServerAutoTestCase

class TestKmlUnrelatedToInference(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--kml-dir": cls.test_dir + "/../../models/catboost",
        })
    def test_store_fetch_delete_serialized(self):
        resp = self.web_server.http_get(uri="/")
        self.assertEqual(200, resp.status_code)
        self.web_server.assert_log(["kml model absent_model not found"],
                                   "Can't find kml \"model not found\" error message")
        
