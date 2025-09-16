import glob
import os
import subprocess
import shutil
import sys

from .file_utils import search_kphp2cpp, error_can_be_ignored, can_ignore_sanitizer_log
from .nocc_for_kphp_tester import nocc_make_env, nocc_prepend_to_cxx


class Artifact:
    def __init__(self, file, error_priority):
        self.file = file
        self.error_priority = error_priority


class KphpBuilder:
    def __init__(self, php_script_path, artifacts_dir, working_dir, use_nocc=False, cxx_name="g++"):
        self.artifacts = {}
        self._kphp_build_stderr_artifact = None
        self._kphp_build_sanitizer_log_artifact = None
        self._kphp_runtime_stderr = None
        self._artifacts_dir = artifacts_dir

        self._kphp_path = os.path.abspath(search_kphp2cpp())
        self._test_file_path = os.path.abspath(php_script_path)
        self._working_dir = working_dir
        self._include_dirs = [os.path.dirname(self._test_file_path)]
        self._kphp_build_tmp_dir = os.path.join(self._working_dir, "kphp_build")
        self._kphp_runtime_bin = os.path.join(self._kphp_build_tmp_dir, "server")
        self._use_nocc = use_nocc
        self._cxx_name = cxx_name

    @property
    def kphp_build_tmp_dir(self):
        return self._kphp_build_tmp_dir

    @property
    def kphp_build_stderr_artifact(self):
        return self._kphp_build_stderr_artifact

    @property
    def kphp_build_sanitizer_log_artifact(self):
        return self._kphp_build_sanitizer_log_artifact

    @property
    def kphp_runtime_stderr(self):
        return self._kphp_runtime_stderr

    @property
    def kphp_runtime_bin(self):
        return self._kphp_runtime_bin

    def _create_artifacts_dir(self):
        os.makedirs(self._artifacts_dir, exist_ok=True)

    def _move_to_artifacts(self, artifact_name, return_code, content=None, file=None):
        self._create_artifacts_dir()
        artifact = Artifact(os.path.join(self._artifacts_dir, artifact_name), abs(return_code))
        self.artifacts[artifact_name.replace("_", " ").replace(".", " ")] = artifact
        if content:
            with open(artifact.file, 'wb') as f:
                f.write(content)
        if file:
            shutil.move(file, artifact.file)
        return artifact

    @staticmethod
    def _wait_proc(proc, timeout=300):
        try:
            stdout, stderr = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            try:
                stdout, stderr = proc.communicate(timeout=timeout)
            except subprocess.TimeoutExpired:
                os.system("kill -9 {}".format(proc.pid))
                return None, b"Zombie detected?! Proc can't be killed due timeout!"

            stderr = (stderr or b"") + b"\n\nKilled due timeout\n"
        return stdout, stderr

    @staticmethod
    def _clear_working_dir(dir_path):
        if os.path.exists(dir_path):
            bad_paths = []
            shutil.rmtree(dir_path, onerror=lambda f, path, e: bad_paths.append(path))

            # some php tests changes permissions
            for bad_path in reversed(bad_paths):
                os.chmod(bad_path, 0o777)
            if bad_paths:
                shutil.rmtree(dir_path)
        os.makedirs(dir_path)

    def remove_artifacts_dir(self):
        shutil.rmtree(self._artifacts_dir, ignore_errors=True)

    def try_remove_kphp_build_trash(self):
        shutil.rmtree(self._kphp_build_tmp_dir, ignore_errors=True)

    @staticmethod
    def _prepare_sanitizer_env(working_directory, sanitizer_log_name, detect_leaks=1):
        tmp_sanitizer_file = os.path.join(working_directory, sanitizer_log_name)
        sanitizer_glob_mask = "{}.*".format(tmp_sanitizer_file)
        for old_sanitizer_file in glob.glob(sanitizer_glob_mask):
            os.remove(old_sanitizer_file)

        env = os.environ.copy()
        env["ASAN_OPTIONS"] = f"detect_leaks={detect_leaks}:allocator_may_return_null=1:log_path={tmp_sanitizer_file}"
        env["UBSAN_OPTIONS"] = f"print_stacktrace=1:allow_addr2line=1:log_path={tmp_sanitizer_file}"
        return env, sanitizer_glob_mask

    def _move_sanitizer_logs_to_artifacts(self, sanitizer_glob_mask, proc, sanitizer_log_name):
        for sanitizer_log in glob.glob(sanitizer_glob_mask):
            if not can_ignore_sanitizer_log(sanitizer_log):
                return self._move_to_artifacts(sanitizer_log_name, proc.returncode, file=sanitizer_log)
        return None

    def compile_with_kphp(self, kphp_env=None):
        os.makedirs(self._kphp_build_tmp_dir, exist_ok=True)
        print("\n!Ccompile_with_kphp step 1")

        sanitizer_log_name = "kphp_build_sanitizer_log"
        env, sanitizer_glob_mask = self._prepare_sanitizer_env(self._kphp_build_tmp_dir, sanitizer_log_name, detect_leaks=0)
        if kphp_env:
            env.update(kphp_env)
        if "KPHP_FUNCTIONS" in env.keys():
            env["KPHP_FUNCTIONS"] = self._test_file_path[:self._test_file_path.rfind('/')] + "/" + env["KPHP_FUNCTIONS"]

        env.setdefault("KPHP_THREADS_COUNT", "3")
        env.setdefault("KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS", "1")
        env.setdefault("KPHP_ENABLE_FULL_PERFORMANCE_ANALYZE", "1")
        env.setdefault("KPHP_PROFILER", "2")
        if sys.platform != "darwin":
            env.setdefault("KPHP_DYNAMIC_INCREMENTAL_LINKAGE", "1")
        if "KPHP_INCLUDE_DIR" in env:
            self._include_dirs = [env["KPHP_INCLUDE_DIR"]] + self._include_dirs
        env["KPHP_INCLUDE_DIR"] = ":".join(self._include_dirs)
        env["KPHP_DEST_DIR"] = os.path.abspath(self._kphp_build_tmp_dir)
        env.setdefault("KPHP_WARNINGS_LEVEL", "2")
        if self._use_nocc:
            print("\n!Ccompile_with_kphp step 2 (use nocc)")
            env.setdefault("KPHP_JOBS_COUNT", "16")
            env.setdefault("KPHP_CXX", nocc_prepend_to_cxx(self._cxx_name))
            env.update(nocc_make_env())
        else:
            print("\n!Ccompile_with_kphp step 2 (not use nocc)")
            env.setdefault("KPHP_CXX", self._cxx_name)
            env.setdefault("KPHP_JOBS_COUNT", "2")

        args = [self._kphp_path, self._test_file_path]

        print("\n!Ccompile_with_kphp step 3: start tmp")
        subprocess.Popen(
            args,
            cwd=self._kphp_build_tmp_dir,
            env=env,
        )
        print("\n!Ccompile_with_kphp step 3: end tmp")




        # TODO kphp writes error into stdout and info into stderr
        kphp_compilation_proc = subprocess.Popen(
            args,
            cwd=self._kphp_build_tmp_dir,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT
        )
        print("\n!Ccompile_with_kphp step 3: args", args)
        print("\n!Ccompile_with_kphp step 3: cwd", self._kphp_build_tmp_dir)
        print("\n!Ccompile_with_kphp step 3: env", env)

        kphp_build_stderr, fake_stderr = self._wait_proc(kphp_compilation_proc, timeout=1200)
        if fake_stderr:
            kphp_build_stderr = (kphp_build_stderr or b'') + fake_stderr

        print("\n!Ccompile_with_kphp step 4")

        self._kphp_build_sanitizer_log_artifact = self._move_sanitizer_logs_to_artifacts(
            sanitizer_glob_mask, kphp_compilation_proc, sanitizer_log_name)

        print("\n!Ccompile_with_kphp step 5")

        ignore_stderr = error_can_be_ignored(
            ignore_patterns=[
                "^Starting php to cpp transpiling\\.\\.\\.$",
                "^Starting make\\.\\.\\.$",
                "^Compiling pch stage started\\.\\.\\.$",
                "^\\[nocc\\] saved pch file to .+$",
                "^objs cnt = \\d+$",
                "^Compiling pch stage started\\.\\.\\.$",
                "^Compiling stage started\\.\\.\\.$",
                "^Linking stage started\\.\\.\\.$",
                "^\\s*\\d+\\% \\[total jobs \\d+\\] \\[left jobs \\d+\\] \\[running jobs \\d+\\] \\[waiting jobs \\d+\\]$"
            ],
            binary_error_text=kphp_build_stderr)

        print("\n!Ccompile_with_kphp step 6")

        if not ignore_stderr:
            return_code = kphp_compilation_proc.returncode
            self._kphp_build_stderr_artifact = self._move_to_artifacts(
                artifact_name="kphp_build_stderr",
                return_code=1 if return_code is None else return_code,
                content=kphp_build_stderr
            )

        print("\n!Ccompile_with_kphp step 7")

        return kphp_compilation_proc.returncode == 0
