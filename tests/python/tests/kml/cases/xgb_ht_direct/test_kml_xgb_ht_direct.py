import os
from python.lib.testcase import WebServerAutoTestCase

class TestKmlHtDirect(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        if cls.should_use_k2():
            os.environ["KPHP_KML_DIR"] = cls.test_dir + "/../../models"
        else:
            cls.web_server.update_options({
                "--kml-dir": cls.test_dir + "/../../models",
            })
    def test(self):
        resp = self.web_server.http_get(uri="/")
        self.assertEqual(200, resp.status_code)
