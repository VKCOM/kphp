from python.lib.testcase import KphpCompilerAutoTestCase


class TestLibs(KphpCompilerAutoTestCase):
    def test_raw_php_libs(self):
        self.build_and_compare_with_php("php/lib_user.php")
