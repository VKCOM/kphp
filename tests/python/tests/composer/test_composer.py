import os
import json
import subprocess

from python.lib.testcase import KphpCompilerAutoTestCase
from python.lib.kphp_builder import KphpBuilder
from python.lib.kphp_server import KphpServer


# test that composer-based project can reference autoloadable
# classes with psr4 prefixes defined in composer.json files
class TestComposer(KphpCompilerAutoTestCase):
    def new_kphp_builder(self, php_script_path = "php/index.php"):
        return KphpBuilder(
            php_script_path=os.path.join(self.test_dir, php_script_path),
            artifacts_dir=self.artifacts_dir,
            working_dir=self.kphp_build_working_dir
        )

    def new_kphp_server(self, builder):
        return KphpServer(
            engine_bin=builder.kphp_runtime_bin,
            working_dir=self.kphp_server_working_dir,
            auto_start=True
        )

    def run_kphp(self, builder):
        kphp_server = self.new_kphp_server(builder)
        return kphp_server.http_get("/")

    def run_php(self, php_script_path = "php/index.php"):
        index_file = os.path.join(self.test_dir, php_script_path)
        cwd = os.path.dirname(index_file)
        try:
            return subprocess.check_output(["php", "-f", index_file], cwd=cwd).decode()
        except subprocess.CalledProcessError as e:
            print("run_php error: " + str(e.output))
            return ""

    def test_psr4_class_loading(self):
        builder = self.new_kphp_builder()
        compilation_result = builder.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php"),
        })
        self.assertTrue(compilation_result)
        self.assertEqual(self.run_kphp(builder).json(), json.loads(self.run_php()))

    def test_psr4_class_loading_no_dev(self):
        # tests/ are not registered in autoloader if we're using -composer-no-dev
        builder = self.new_kphp_builder()
        compilation_result = builder.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php"),
            "KPHP_COMPOSER_NO_DEV": "1",
        })
        self.assertFalse(compilation_result)
        with open(builder.kphp_build_stderr_artifact.file, "r") as error_file:
            self.assertRegex(error_file.read(), r"Class VK\\Feed\\FeedTester not found")

    def test_autoload_files(self):
        builder = self.new_kphp_builder("php/test_autoload_files/index.php")
        compilation_result = builder.compile_with_kphp({
            "KPHP_COMPOSER_ROOT": os.path.join(self.test_dir, "php/test_autoload_files"),
        })
        self.assertTrue(compilation_result)
        self.assertEqual(self.run_kphp(builder).text, self.run_php("php/test_autoload_files/index.php"))
