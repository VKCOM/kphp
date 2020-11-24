import os

from python.lib.testcase import KphpCompilerAutoTestCase
from python.lib.kphp_builder import KphpBuilder
from python.lib.kphp_server import KphpServer


class TestGlobalVarsMemoryStats(KphpCompilerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_builder = KphpBuilder(
            php_script_path=os.path.join(cls.test_dir, "php/global_vars_memory_stats.php"),
            artifacts_dir=cls.artifacts_dir,
            working_dir=cls.kphp_build_working_dir
        )

    def test_get_global_vars_memory_stats_without_option(self):
        compilation_result = self.kphp_builder.compile_with_kphp({
            "KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS": "0"
        })
        self.assertFalse(compilation_result)
        with open(self.kphp_builder.kphp_build_stderr_artifact.file, 'r') as error_file:
            self.assertRegex(error_file.read(), "use KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS to enable")

    def test_get_global_vars_memory_stats_with_option(self):
        compilation_result = self.kphp_builder.compile_with_kphp({
            "KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS": "1"
        })
        self.assertTrue(compilation_result)
        kphp_server = KphpServer(
            engine_bin=self.kphp_builder.kphp_runtime_bin,
            working_dir=self.kphp_server_working_dir,
            auto_start=True
        )
        self.assertEqual(kphp_server.http_get("/").json(), {
            'globals': {'$arr': 104, '$result': 896, '$str': 24},
            'initial': [],
            'static': {
                '$arr': 104, '$result': 1536, '$str': 24,
                'function_with_static_var::$static_array': 184,
                'function_with_static_var::$static_str': 24
            }
        })
