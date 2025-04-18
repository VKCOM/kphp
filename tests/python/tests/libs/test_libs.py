import os
import re
import shutil
import pytest

from python.lib.testcase import KphpCompilerAutoTestCase


class TestLibs(KphpCompilerAutoTestCase):
    def test_raw_php_libs(self):
        self.build_and_compare_with_php("php/lib_user.php")

    @pytest.mark.k2_skip
    def test_compiled_libs(self):
        for lib_name in ("example1", "example2"):
            lib_out_dir = os.path.join(self.web_server_working_dir, "lib_examples/{}/lib".format(lib_name))
            lib_builder = self.make_kphp_once_runner("php/lib_examples/{}".format(lib_name))
            self.assertTrue(lib_builder.compile_with_kphp({
                "KPHP_MODE": "lib",
                "KPHP_DYNAMIC_INCREMENTAL_LINKAGE": "0",
                "KPHP_OUT_LIB_DIR": lib_out_dir
            }), "Got {} KPHP build error".format(lib_name))

        kphp_runner = self.build_and_compare_with_php("php/lib_user.php", kphp_env={
            "KPHP_INCLUDE_DIR": self.web_server_working_dir,
            "KPHP_VERBOSITY": "3",
        })
        with open(kphp_runner.kphp_build_stderr_artifact.file, "rb") as f:
            build_log = f.read()
        self.assertRegex(build_log, re.compile(b"Use static lib \\[.*lib_examples/example1/lib/libexample1\\.a]"))
        self.assertRegex(build_log, re.compile(b"Use static lib \\[.*lib_examples/example2/lib/libexample2\\.a]"))

        shutil.rmtree(os.path.join(self.web_server_working_dir, "lib_examples"), ignore_errors=True)
