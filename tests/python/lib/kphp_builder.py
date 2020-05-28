import copy
import glob
import os
import subprocess
import shutil

from .file_utils import search_kphp_sh, error_can_be_ignored, can_ignore_asan_log


class Artifact:
    def __init__(self, file, error_priority):
        self.file = file
        self.error_priority = error_priority


class KphpBuilder:
    def __init__(self, php_script_path, artifacts_dir, working_dir, kphp_path=None, distcc_hosts=None):
        self.artifacts = {}
        self._kphp_build_stderr_artifact = None
        self._kphp_build_asan_log_artifact = None
        self._artifacts_dir = artifacts_dir

        self._kphp_path = os.path.abspath(kphp_path or search_kphp_sh())
        self._test_file_path = os.path.abspath(php_script_path)
        self._working_dir = working_dir
        self._include_dirs = [os.path.dirname(self._test_file_path)]
        self._kphp_build_tmp_dir = os.path.join(self._working_dir, "kphp_build")
        self._kphp_runtime_bin = os.path.join(self._kphp_build_tmp_dir, "server")
        self._distcc_hosts = distcc_hosts or []

    @property
    def kphp_build_stderr_artifact(self):
        return self._kphp_build_stderr_artifact

    @property
    def kphp_build_asan_log_artifact(self):
        return self._kphp_build_asan_log_artifact

    @property
    def kphp_runtime_bin(self):
        return self._kphp_runtime_bin

    def _create_artifacts_dir(self):
        os.makedirs(self._artifacts_dir, exist_ok=True)

    def _move_to_artifacts(self, artifact_name, priority, content=None, file=None):
        self._create_artifacts_dir()
        artifact = Artifact(os.path.join(self._artifacts_dir, artifact_name), priority)
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

    def try_remove_kphp_runtime_bin(self):
        try:
            os.remove(self._kphp_runtime_bin)
        except OSError:
            pass

    @staticmethod
    def _prepare_asan_env(working_directory, asan_log_name):
        tmp_asan_file = os.path.join(working_directory, asan_log_name)
        asan_glob_mask = "{}.*".format(tmp_asan_file)
        for old_asan_file in glob.glob(asan_glob_mask):
            os.remove(old_asan_file)

        env = copy.copy(os.environ)
        env["ASAN_OPTIONS"] = "detect_leaks=0:log_path={}".format(tmp_asan_file)
        return env, asan_glob_mask

    def _move_asan_logs_to_artifacts(self, asan_glob_mask, proc, asan_log_name):
        for asan_log in glob.glob(asan_glob_mask):
            if not can_ignore_asan_log(asan_log):
                return self._move_to_artifacts(asan_log_name, proc.returncode, file=asan_log)
        return None

    def compile_with_kphp(self, use_dynamic_incremental_linkage=True):
        os.makedirs(self._kphp_build_tmp_dir, exist_ok=True)

        asan_log_name = "kphp_build_asan_log"
        env, asan_glob_mask = self._prepare_asan_env(self._kphp_build_tmp_dir, asan_log_name)
        env["KPHP_THREADS_COUNT"] = "3"
        env["KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS"] = "1"
        env["GDB_OPTION"] = "-g0"
        env["KPHP_DYNAMIC_INCREMENTAL_LINKAGE"] = use_dynamic_incremental_linkage and "1" or "0"
        if self._distcc_hosts:
            env["KPHP_JOBS_COUNT"] = "30"
            env["DISTCC_HOSTS"] = " ".join(self._distcc_hosts)
            env["DISTCC_DIR"] = self._kphp_build_tmp_dir
            env["DISTCC_LOG"] = os.path.join(self._kphp_build_tmp_dir, "distcc.log")
            env["CXX"] = "distcc x86_64-linux-gnu-g++"
        else:
            env["KPHP_JOBS_COUNT"] = "2"

        include = " ".join("-I {}".format(include_dir) for include_dir in self._include_dirs)
        # TODO kphp writes error into stdout and info into stderr
        kphp_compilation_proc = subprocess.Popen(
            [self._kphp_path, include, "-d", os.path.abspath(self._kphp_build_tmp_dir), self._test_file_path],
            cwd=self._kphp_build_tmp_dir,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT
        )
        kphp_build_stderr, fake_stderr = self._wait_proc(kphp_compilation_proc, timeout=1200)
        if fake_stderr:
            kphp_build_stderr = (kphp_build_stderr or b'') + fake_stderr

        self._kphp_build_asan_log_artifact = self._move_asan_logs_to_artifacts(
            asan_glob_mask, kphp_compilation_proc, asan_log_name)
        ignore_stderr = error_can_be_ignored(
            ignore_patterns=[
                "^Starting php to cpp transpiling\\.\\.\\.$",
                "^Starting make\\.\\.\\.$",
                "^objs cnt = \\d+$",
                "^\\s*\\d+\\% \\[total jobs \\d+\\] \\[left jobs \\d+\\] \\[running jobs \\d+\\] \\[waiting jobs \\d+\\]$"
            ],
            binary_error_text=kphp_build_stderr)
        if not ignore_stderr:
            priority = kphp_compilation_proc.returncode
            self._kphp_build_stderr_artifact = self._move_to_artifacts(
                artifact_name="kphp_build_stderr",
                priority=1 if priority is None else priority,
                content=kphp_build_stderr
            )

        return kphp_compilation_proc.returncode == 0
