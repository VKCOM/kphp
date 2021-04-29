import os

from python.lib.testcase import KphpCompilerAutoTestCase


# test that composer-based project can reference autoloadable
# classes with psr4 prefixes defined in composer.json files
class TestComposer(KphpCompilerAutoTestCase):
    def test_psr4_class_loading(self):
        self.build_and_compare_with_php(
            php_script_path="php/index.php",
            kphp_env={
                "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php")
            })

    def test_psr4_class_loading_no_dev(self):
        once_runner = self.make_kphp_once_runner("php/index.php")
        # tests/ are not registered in autoloader if we're using -composer-no-dev
        self.assertFalse(once_runner.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php"),
            "KPHP_COMPOSER_NO_DEV": "1",
        }))
        with open(once_runner.kphp_build_stderr_artifact.file, "r") as error_file:
            self.assertRegex(error_file.read(), r"Class VK\\Feed\\FeedTester not found")

    def test_autoload_files(self):
        self.build_and_compare_with_php(
            php_script_path="php/test_autoload_files/index.php",
            kphp_env={
                "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php/test_autoload_files")
            })
