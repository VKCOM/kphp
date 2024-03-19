import os
import shutil
import subprocess
import sys

from .kphp_builder import KphpBuilder
from .file_utils import error_can_be_ignored


class KphpRunOnce(KphpBuilder):
    def __init__(self, php_script_path, artifacts_dir, working_dir, php_bin,
                 extra_include_dirs=None, vkext_dir=None, use_nocc=False, cxx_name="g++"):
        super(KphpRunOnce, self).__init__(
            php_script_path=php_script_path,
            artifacts_dir=artifacts_dir,
            working_dir=working_dir,
            use_nocc=use_nocc,
            cxx_name=cxx_name,
        )

        self._php_stdout = None
        self._kphp_server_stdout = None
        self._php_tmp_dir = os.path.join(self._working_dir, "php")
        self._kphp_runtime_tmp_dir = os.path.join(self._working_dir, "kphp_runtime")
        if extra_include_dirs:
            self._include_dirs.extend(extra_include_dirs)
        self._vkext_dir = vkext_dir
        self._php_bin = php_bin

    def _get_extensions(self):
        if sys.platform == "darwin":
            extensions = []
        else:
            extensions = [
                ("extension", "json.so"),
                ("extension", "bcmath.so"),
                ("extension", "iconv.so"),
                ("extension", "mbstring.so"),
                ("extension", "curl.so"),
                ("extension", "tokenizer.so"),
                ("extension", "h3.so"),
                ("extension", "zstd.so"),
                ("extension", "ctype.so")
            ]

        if self._vkext_dir:
            vkext_so = None
            if self._php_bin.endswith("php7.2"):
                vkext_so = os.path.join(self._vkext_dir, "modules7.2", "vkext.so")
            elif self._php_bin.endswith("php7.4"):
                vkext_so = os.path.join(self._vkext_dir, "modules7.4", "vkext.so")
            elif sys.platform == "darwin":
                vkext_so = os.path.join(self._vkext_dir, "modules", "vkext.dylib")

            if not vkext_so or not os.path.exists(vkext_so):
                vkext_so = "vkext.so"
            extensions.append(("extension", vkext_so))

        return extensions

    def run_with_php(self, extra_options=[], runs_cnt=1):
        self._clear_working_dir(self._php_tmp_dir)
        options = self._get_extensions()
        options.extend(extra_options)
        options.extend([
            ("display_errors", 0),
            ("log_errors", 1),
            ("memory_limit", "3072M"),
            ("xdebug.var_display_max_depth", -1),
            ("xdebug.var_display_max_children", -1),
            ("xdebug.var_display_max_data", -1),
            ("include_path", ":".join(self._include_dirs))
        ])

        cmd = [self._php_bin, "-n"]
        for k, v in options:
            cmd.append("-d {}='{}'".format(k, v))
        cmd.append(self._test_file_path)
        php_proc = subprocess.Popen(cmd, cwd=self._php_tmp_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self._php_stdout, php_stderr = self._wait_proc(php_proc)

        # We can assume that php is always idempotent and just copy output instead of running php script several times
        self._php_stdout *= runs_cnt

        if php_stderr:
            if 'GITHUB_ACTIONS' in os.environ:
                print("php_stderr: " + str(php_stderr))
            else:
                self._move_to_artifacts("php_stderr", php_proc.returncode, content=php_stderr)

        if not os.listdir(self._php_tmp_dir):
            shutil.rmtree(self._php_tmp_dir, ignore_errors=True)

        return php_proc.returncode == 0

    def run_with_kphp(self, runs_cnt=1, args=[]):
        self._clear_working_dir(self._kphp_runtime_tmp_dir)

        sanitizer_log_name = "kphp_runtime_sanitizer_log"
        env, sanitizer_glob_mask = self._prepare_sanitizer_env(self._kphp_runtime_tmp_dir, sanitizer_log_name)

        cmd = [self._kphp_runtime_bin, "--once={}".format(runs_cnt), "--profiler-log-prefix", "profiler.log",
               "--worker-queries-to-reload", "1"] + args
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
            self._kphp_runtime_stderr = self._move_to_artifacts(
                "kphp_runtime_stderr",
                kphp_server_proc.returncode,
                content=kphp_runtime_stderr)

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
            if 'GITHUB_ACTIONS' in os.environ:
                # just open and read the file - it's easier than messing with popen, etc.
                with open(diff_artifact.file, 'r') as f:
                    print('diff: ' + f.read())
        with open(diff_artifact.file, 'r') as f:
            print(f.read())

        return False
