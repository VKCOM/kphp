import os
from python.lib.testcase import WebServerAutoTestCase

class TestKmlUnrelatedToInference(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if cls.should_use_k2():
            os.environ["KPHP_KML_DIR"] = cls.test_dir + "/../../models/catboost"
        else:
            cls.web_server.update_options({
                "--kml-dir": cls.test_dir + "/../../models/catboost",
            })
    def test(self):
        resp = self.web_server.http_get(uri="/")
        self.assertEqual(200, resp.status_code)
        self.web_server.assert_log(["kml model absent_model not found"],
                                   "Can't find kml \"model not found\" error message")
        
