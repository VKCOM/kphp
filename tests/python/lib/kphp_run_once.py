import os
import shutil
import subprocess
import sys

from .kphp_builder import KphpBuilder
from .file_utils import error_can_be_ignored


class TestFile:
    def __init__(self, file_path, test_tmp_dir, tags, env_vars: dict, out_regexps=None, forbidden_regexps=None):
        self.test_tmp_dir = test_tmp_dir
        self.file_path = file_path
        self.tags = tags
        self.env_vars = env_vars
        self.out_regexps = out_regexps
        self.forbidden_regexps = forbidden_regexps

    def is_ok(self):
        return "ok" in self.tags

    def is_kphp_should_fail(self):
        return "kphp_should_fail" in self.tags

    def is_kphp_should_warn(self):
        return "kphp_should_warn" in self.tags

    def is_php5(self):
        return "php5" in self.tags

    def is_php7_4(self):
        return "php7_4" in self.tags


class KphpRunOnce(KphpBuilder):
    def __init__(self, test_file: TestFile, distcc_hosts):
        super(KphpRunOnce, self).__init__(
            php_script_path=test_file.file_path,
            artifacts_dir=os.path.join(test_file.test_tmp_dir, "artifacts"),
            working_dir=os.path.abspath(os.path.join(test_file.test_tmp_dir, "working_dir")),
            distcc_hosts=distcc_hosts
        )

        self._test_file = test_file
        self._php_stdout = None
        self._kphp_server_stdout = None
        self._php_tmp_dir = os.path.join(self._working_dir, "php")
        self._kphp_runtime_tmp_dir = os.path.join(self._working_dir, "kphp_runtime")
        self._tester_dir = os.path.abspath(os.path.dirname(__file__))
        self._include_dirs.append(os.path.join(self._tester_dir, "php_include"))
        self._vkext_dir = os.path.abspath(os.path.join(self._tester_dir, os.path.pardir, "objs", "vkext"))

    def _get_php_bin(self):
        if sys.platform == "darwin":
            return shutil.which("php")
        if self._test_file.is_php5():
            return shutil.which("php5.6") or shutil.which("php5")
        if self._test_file.is_php7_4():
            return shutil.which("php7.4")
        return shutil.which("php7.2") or shutil.which("php7.3") or shutil.which("php7.4")

    def _get_extensions_and_php_path(self):
        php_bin = self._get_php_bin()
        if php_bin is None:
            raise RuntimeError("Can't find php executable")

        if sys.platform == "darwin":
            return php_bin, []

        vkext_so = None
        if php_bin.endswith("php7.2"):
            vkext_so = os.path.join(self._vkext_dir, "modules7.2", "vkext.so")
        elif php_bin.endswith("php7.4"):
            vkext_so = os.path.join(self._vkext_dir, "modules7.4", "vkext.so")

        if not vkext_so or not os.path.exists(vkext_so):
            vkext_so = "vkext.so"

        extensions = [
            ("extension", "json.so"),
            ("extension", "bcmath.so"),
            ("extension", "iconv.so"),
            ("extension", "mbstring.so"),
            ("extension", "curl.so"),
            ("extension", "tokenizer.so"),
            ("extension", "h3.so"),
            ("extension", "zstd.so"),
            ("extension", vkext_so)
        ]
        return php_bin, extensions

    def run_with_php(self):
        self._clear_working_dir(self._php_tmp_dir)
        php_bin, options = self._get_extensions_and_php_path()
        options.extend([
            ("display_errors", 0),
            ("log_errors", 1),
            ("memory_limit", "3072M"),
            ("xdebug.var_display_max_depth", -1),
            ("xdebug.var_display_max_children", -1),
            ("xdebug.var_display_max_data", -1),
            ("include_path", "{}:{}".format(*self._include_dirs))
        ])

        cmd = [php_bin, "-n"]
        for k, v in options:
            cmd.append("-d {}='{}'".format(k, v))
        cmd.append(self._test_file_path)
        php_proc = subprocess.Popen(cmd, cwd=self._php_tmp_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self._php_stdout, php_stderr = self._wait_proc(php_proc)

        if php_stderr:
            self._move_to_artifacts("php_stderr", php_proc.returncode, content=php_stderr)

        if not os.listdir(self._php_tmp_dir):
            shutil.rmtree(self._php_tmp_dir, ignore_errors=True)

        return php_proc.returncode == 0

    def run_with_kphp(self):
        self._clear_working_dir(self._kphp_runtime_tmp_dir)

        sanitizer_log_name = "kphp_runtime_sanitizer_log"
        env, sanitizer_glob_mask = self._prepare_sanitizer_env(self._kphp_runtime_tmp_dir, sanitizer_log_name)

        cmd = [self._kphp_runtime_bin, "-o", "--disable-sql", "--profiler-log-prefix", "profiler.log"]
        if not os.getuid():
            cmd += ["-u", "root", "-g", "root"]
        kphp_server_proc = subprocess.Popen(cmd,
                                            cwd=self._kphp_runtime_tmp_dir,
                                            env=env,
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE)
        self._kphp_server_stdout, kphp_runtime_stderr = self._wait_proc(kphp_server_proc)

        self._move_sanitizer_logs_to_artifacts(sanitizer_glob_mask, kphp_server_proc, sanitizer_log_name)
        ignore_stderr = error_can_be_ignored(
            ignore_patterns=[
                "^\\[\\d+\\]\\[\\d{4}\\-\\d{2}\\-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d+ php\\-runner\\.cpp\\s+\\d+\\].+$"
            ],
            binary_error_text=kphp_runtime_stderr)

        if not ignore_stderr:
            self._move_to_artifacts("kphp_runtime_stderr", kphp_server_proc.returncode, content=kphp_runtime_stderr)

        return kphp_server_proc.returncode == 0

    def compare_php_and_kphp_stdout(self):
        if self._kphp_server_stdout == self._php_stdout:
            return True

        diff_artifact = self._move_to_artifacts("php_vs_kphp.diff", 1, b"TODO")
        php_stdout_file = os.path.join(self._artifacts_dir, "php_stdout")
        with open(php_stdout_file, 'wb') as f:
            f.write(self._php_stdout)
        kphp_server_stdout_file = os.path.join(self._artifacts_dir, "kphp_server_stdout")
        with open(kphp_server_stdout_file, 'wb') as f:
            f.write(self._kphp_server_stdout)

        with open(diff_artifact.file, 'wb') as f:
            subprocess.call(["diff", "--text", "-ud", php_stdout_file, kphp_server_stdout_file], stdout=f)

        return False
