import os

from python.lib.testcase import KphpCompilerAutoTestCase


class TestKML(KphpCompilerAutoTestCase):
    def compile_and_check_success(self, php_script_path, kml_dir):
        kml_dir = os.path.join(self.test_dir, kml_dir)
        args = ["--kml-dir={}".format(kml_dir)]

        once_runner = self.make_kphp_once_runner(php_script_path)

        self.assertTrue(once_runner.compile_with_kphp({}))
        self.assertTrue(once_runner.run_with_kphp(args=args), "Got KPHP runtime error")

    def test_xgboost_tiny_ht_direct_int_keys_to_fvalue(self):
        self.compile_and_check_success(php_script_path="xgboost/php/xgb_tiny_ht_direct_int_keys_to_fvalue.php",
                                       kml_dir="xgboost/kml")
        self.compile_and_check_success(php_script_path="xgboost/php/xgb_tiny_ht_direct_int_keys_to_fvalue.php",
                                       kml_dir="xgboost")  # Traverse directories recursively

    def test_xgb_tiny_ht_remap_str_keys_to_fvalue(self):
        self.compile_and_check_success(php_script_path="xgboost/php/xgb_tiny_ht_remap_str_keys_to_fvalue.php",
                                       kml_dir="xgboost/kml")

    def test_catboost_tiny_1float_1hot_10trees(self):
        self.compile_and_check_success(php_script_path="catboost/php/catboost_tiny_1float_1hot_10trees.php",
                                       kml_dir="catboost/kml")

    def test_catboost_multiclass_tutorial_ht_catnum(self):
        self.compile_and_check_success(php_script_path="catboost/php/catboost_multiclass_tutorial_ht_catnum.php",
                                       kml_dir="catboost/kml")

    def test_unrelated_to_inference(self):
        self.compile_and_check_success(php_script_path="catboost/php/unrelated_to_inference.php",
                                       kml_dir="catboost/kml")
