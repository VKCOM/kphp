import os

from python.lib.testcase import KphpCompilerAutoTestCase


class TestKML(KphpCompilerAutoTestCase):
    def test_xgboost_tiny(self):
        php_script_path = "xgboost/xgb_tiny_ht_direct_int_keys_to_fvalue.php"
        kml_dir = os.path.join(self.test_dir, "xgboost")
        args = ["--kml-dir={}".format(kml_dir)]

        once_runner = self.make_kphp_once_runner(php_script_path)

        self.assertTrue(once_runner.compile_with_kphp({}))
        self.assertTrue(once_runner.run_with_kphp(args=args), "Got KPHP runtime error")
