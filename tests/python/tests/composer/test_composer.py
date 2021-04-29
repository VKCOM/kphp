import os
import json
import subprocess

from python.lib.testcase import KphpCompilerAutoTestCase


# test that composer-based project can reference autoloadable
# classes with psr4 prefixes defined in composer.json files
class TestComposer(KphpCompilerAutoTestCase):
    def test_psr4_class_loading(self):
        once_runner = self.make_kphp_once_runner("php/index.php")
        self.assertTrue(once_runner.run_with_php())
        self.assertTrue(once_runner.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php")
        }))
        self.assertTrue(once_runner.run_with_kphp())
        self.assertTrue(once_runner.compare_php_and_kphp_stdout())

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
        once_runner = self.make_kphp_once_runner("php/test_autoload_files/index.php")
        self.assertTrue(once_runner.run_with_php())
        self.assertTrue(once_runner.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php/test_autoload_files"),
        }))
        self.assertTrue(once_runner.run_with_kphp())
        self.assertTrue(once_runner.compare_php_and_kphp_stdout())
