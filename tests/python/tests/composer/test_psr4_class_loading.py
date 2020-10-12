import os
import json
import subprocess

from python.lib.testcase import KphpCompilerAutoTestCase
from python.lib.kphp_builder import KphpBuilder
from python.lib.kphp_server import KphpServer


# test that composer-based project can reference autoloadable
# classes with psr4 prefixes defined in composer.json files
class TestPsr4ClassLoading(KphpCompilerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_builder = KphpBuilder(
            php_script_path=os.path.join(cls.test_dir, "php/index.php"),
            artifacts_dir=cls.artifacts_dir,
            working_dir=cls.kphp_build_working_dir
        )

    def run_php(self):
        cwd = os.path.join(self.test_dir, "php")
        index_file = os.path.join(self.test_dir, "php/index.php")
        return subprocess.check_output(["php", "-f", index_file], cwd=cwd).decode()

    def test_psr4_class_loading(self):
        compilation_result = self.kphp_builder.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php"),
        })
        self.assertTrue(compilation_result)
        kphp_server = KphpServer(
            engine_bin=self.kphp_builder.kphp_runtime_bin,
            working_dir=self.kphp_server_working_dir,
            auto_start=True
        )
        php_result = self.run_php()
        self.assertEqual(kphp_server.http_get("/").json(), json.loads(php_result))

    def test_psr4_class_loading_no_dev(self):
        # tests/ are not registered in autoloader if we're using -composer-no-dev
        compilation_result = self.kphp_builder.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php"),
            "KPHP_COMPOSER_NO_DEV": "1",
        })
        self.assertFalse(compilation_result)
        with open(self.kphp_builder.kphp_build_stderr_artifact.file, "r") as error_file:
            self.assertRegex(error_file.read(), r"Class VK\\Feed\\FeedTester not found")
