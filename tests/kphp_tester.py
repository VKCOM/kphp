#!/usr/bin/python3
import argparse
import math
import multiprocessing
import os
import re
import shutil
import signal
import subprocess
import sys
from functools import partial
from multiprocessing.dummy import Pool as ThreadPool

from python.lib.colors import red, green, yellow, blue
from python.lib.kphp_builder import KphpBuilder
from python.lib.file_utils import read_distcc_hosts, error_can_be_ignored


class TestFile:
    def __init__(self, file_path, test_tmp_dir, tags, env_vars: dict, out_regexps=None):
        self.test_tmp_dir = test_tmp_dir
        self.file_path = file_path
        self.tags = tags
        self.env_vars = env_vars
        self.out_regexps = out_regexps

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


class KphpRunOnceRunner(KphpBuilder):
    def __init__(self, test_file: TestFile, kphp_path, distcc_hosts):
        super(KphpRunOnceRunner, self).__init__(
            php_script_path=test_file.file_path,
            artifacts_dir=os.path.join(test_file.test_tmp_dir, "artifacts"),
            working_dir=os.path.abspath(os.path.join(test_file.test_tmp_dir, "working_dir")),
            kphp_path=kphp_path,
            distcc_hosts=distcc_hosts
        )

        self._test_file = test_file
        self._php_stdout = None
        self._kphp_server_stdout = None
        self._php_tmp_dir = os.path.join(self._working_dir, "php")
        self._kphp_runtime_tmp_dir = os.path.join(self._working_dir, "kphp_runtime")
        self._include_dirs.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "./php_include")))

    def run_with_php(self):
        self._clear_working_dir(self._php_tmp_dir)

        if self._test_file.is_php5():
            php_bin = shutil.which("php5.6") or shutil.which("php5")
        elif self._test_file.is_php7_4():
            php_bin = shutil.which("php7.4")
        else:
            php_bin = shutil.which("php7.2") or shutil.which("php7.3") or shutil.which("php7.4")

        if php_bin is None:
            raise RuntimeError("Can't find php executable")

        options = [
            ("display_errors", 0),
            ("log_errors", 1),
            ("error_log", "/proc/self/fd/2"),
            ("extension", "json.so"),
            ("extension", "bcmath.so"),
            ("extension", "iconv.so"),
            ("extension", "mbstring.so"),
            ("extension", "curl.so"),
            ("extension", "vkext.so"),
            ("extension", "tokenizer.so"),
            ("extension", "h3.so"),
            ("memory_limit", "3072M"),
            ("xdebug.var_display_max_depth", -1),
            ("xdebug.var_display_max_children", -1),
            ("xdebug.var_display_max_data", -1),
            ("include_path", "{}:{}".format(*self._include_dirs))
        ]

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
                "^\\[\\d+\\]\\[\\d{4}\\-\\d{2}\\-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d+ [\w\d/_-]+/PHP/server/php\\-runner\\.cpp\\s+\\d+\\].+$"
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


def make_test_file(file_path, test_tmp_dir, test_tags):
    # if file doesn't exist it will fail late
    if file_path.endswith(".phpt") or not os.path.exists(file_path):
        test_acceptable = True
        for test_tag in test_tags:
            test_acceptable = file_path.find(test_tag) != -1
            if test_acceptable:
                break
        if not test_acceptable:
            return None

        return TestFile(file_path, test_tmp_dir, ["ok"], {})

    with open(file_path, 'rb') as f:
        first_line = f.readline().decode('utf-8')
        if not first_line.startswith("@"):
            return None

        tags = first_line[1:].split()
        test_acceptable = True
        for test_tag in test_tags:
            test_acceptable = (test_tag in tags) or (file_path.find(test_tag) != -1)
            if test_acceptable:
                break

        if not test_acceptable:
            return None

        out_regexps = []
        env_vars = {}  # string -> string, e.g. {"KPHP_REQUIRE_FUNCTIONS_TYPING" -> "1"}
        while True:
            second_line = f.readline().decode('utf-8').strip()
            if len(second_line) > 1 and second_line.startswith("/") and second_line.endswith("/"):
                out_regexps.append(re.compile(second_line[1:-1]))
            elif second_line.startswith("KPHP_") and "=" in second_line:
                env_name, env_value = parse_env_var_from_test_header(file_path, second_line)
                env_vars[env_name] = env_value
            else:  # <?php
                break

        return TestFile(file_path, test_tmp_dir, tags, env_vars, out_regexps)


def parse_env_var_from_test_header(file_path, line):
    env_name, env_value = line.split('=', 2)
    # todo some placeholders in env_value? like {dir} for example
    return env_name, env_value


def test_files_from_dir(tests_dir):
    for root, _, files in os.walk(tests_dir):
        for file in files:
            yield root, file


def test_files_from_list(tests_dir, test_list):
    with open(test_list, encoding='utf-8') as f:
        for line in f.readlines():
            idx = line.find("#")
            if idx != -1:
                line = line[:idx]
            line = line.strip()
            if not line:
                continue
            yield tests_dir, line


def collect_tests(tests_dir, test_tags, test_list):
    tests = []
    tmp_dir = "{}_tmp".format(__file__[:-3])
    file_it = test_files_from_list(tests_dir, test_list) if test_list else test_files_from_dir(tests_dir)
    for root, file in file_it:
        if file.endswith(".php") or file.endswith(".phpt"):
            test_file_path = os.path.join(root, file)
            test_tmp_dir = os.path.join(tmp_dir, os.path.relpath(test_file_path, os.path.dirname(tests_dir)))
            test_tmp_dir = test_tmp_dir[:-4] if test_tmp_dir.endswith(".php") else test_tmp_dir[:-5]
            test_file = make_test_file(test_file_path, test_tmp_dir, test_tags)
            if test_file:
                tests.append(test_file)

    tests.sort(reverse=True, key=lambda f: os.path.getsize(f.file_path))
    return tests


class TestResult:
    @staticmethod
    def failed(test_file, artifacts, failed_stage):
        return TestResult(red("failed "), test_file, artifacts, failed_stage)

    @staticmethod
    def passed(test_file, artifacts):
        return TestResult(green("passed "), test_file, artifacts, None)

    @staticmethod
    def skipped(test_file):
        return TestResult(yellow("skipped"), test_file, None, None)

    def __init__(self, status, test_file, artifacts, failed_stage):
        self.status = status
        self.test_file_path = test_file.file_path
        self.artifacts = None
        if artifacts is not None:
            self.artifacts = [(name, a) for name, a in artifacts.items() if a.error_priority >= 0]
            self.artifacts.sort(key=lambda x: x[1].error_priority, reverse=True)

        self.failed_stage_msg = None
        if failed_stage:
            self.failed_stage_msg = red("({})".format(failed_stage))

    def _print_artifacts(self):
        if self.artifacts:
            for file_type, artifact in self.artifacts:
                file_type_colored = red(file_type) if artifact.error_priority else yellow(file_type)
                print("  {} - {}".format(blue(artifact.file), file_type_colored), flush=True)

    def print_short_report(self, total_tests, test_number):
        width = 1 + int(math.log10(total_tests))
        completed_str = "{0: >{width}}".format(test_number, width=width)
        additional_info = ""
        if self.failed_stage_msg:
            additional_info = self.failed_stage_msg
        elif self.artifacts:
            stderr_names = ", ".join(file_type for file_type, _ in self.artifacts)
            if stderr_names:
                additional_info = yellow("(got {})".format(stderr_names))

        print("[{test_number}/{total_tests}] {status} {test_file} {additional_info}".format(
            test_number=completed_str,
            total_tests=total_tests,
            status=self.status,
            test_file=self.test_file_path,
            additional_info=additional_info), flush=True)

        self._print_artifacts()

    def print_fail_report(self):
        if self.failed_stage_msg:
            print("{} {}".format(self.test_file_path, self.failed_stage_msg), flush=True)
            self._print_artifacts()

    def is_skipped(self):
        return self.artifacts is None

    def is_failed(self):
        return self.failed_stage_msg is not None


def run_fail_test(test: TestFile, runner):
    if runner.compile_with_kphp(test.env_vars):
        return TestResult.failed(test, runner.artifacts, "kphp build is ok, but it expected to fail")

    if test.out_regexps:
        if not runner.kphp_build_stderr_artifact:
            return TestResult.failed(test, runner.artifacts, "kphp build failed without stderr")

        with open(runner.kphp_build_stderr_artifact.file) as f:
            stderr_log = f.read()
            for index, msg_regex in enumerate(test.out_regexps, start=1):
                if not msg_regex.search(stderr_log):
                    return TestResult.failed(test, runner.artifacts, "can't find {}th error pattern".format(index))

    if runner.kphp_build_sanitizer_log_artifact:
        return TestResult.failed(test, runner.artifacts, "got sanitizer log")

    runner.kphp_build_stderr_artifact.error_priority = -1
    return TestResult.passed(test, runner.artifacts)


def run_warn_test(test: TestFile, runner):
    if not runner.compile_with_kphp(test.env_vars):
        return TestResult.failed(test, runner.artifacts, "got kphp build error")
    if not runner.kphp_build_stderr_artifact:
        return TestResult.failed(test, runner.artifacts, "kphp build finished without stderr")

    with open(runner.kphp_build_stderr_artifact.file) as f:
        stderr_log = f.read()
        if not re.search("Warning ", stderr_log):
            return TestResult.failed(test, runner.artifacts, "can't find kphp warnings")
        for index, msg_regex in enumerate(test.out_regexps, start=1):
            if not msg_regex.search(stderr_log):
                return TestResult.failed(test, runner.artifacts, "can't find {}th warning pattern".format(index))

    if runner.kphp_build_sanitizer_log_artifact:
        return TestResult.failed(test, runner.artifacts, "got sanitizer log")

    runner.kphp_build_stderr_artifact.error_priority = -1
    return TestResult.passed(test, runner.artifacts)


def run_ok_test(test: TestFile, runner):
    if not runner.run_with_php():
        return TestResult.failed(test, runner.artifacts, "got php error")
    if not runner.compile_with_kphp(test.env_vars):
        return TestResult.failed(test, runner.artifacts, "got kphp build error")
    if not runner.run_with_kphp():
        return TestResult.failed(test, runner.artifacts, "got kphp runtime error")
    if not runner.compare_php_and_kphp_stdout():
        return TestResult.failed(test, runner.artifacts, "got php and kphp diff")

    return TestResult.passed(test, runner.artifacts)


def run_test(kphp_path, distcc_hosts, test):
    if not os.path.exists(test.file_path):
        return TestResult.failed(test, None, "can't find test file")

    runner = KphpRunOnceRunner(test, kphp_path, distcc_hosts)
    runner.remove_artifacts_dir()

    if test.is_kphp_should_fail():
        test_result = run_fail_test(test, runner)
    elif test.is_kphp_should_warn():
        test_result = run_warn_test(test, runner)
    elif test.is_ok():
        test_result = run_ok_test(test, runner)
    else:
        test_result = TestResult.skipped(test)

    runner.try_remove_kphp_build_trash()

    return test_result


def run_all_tests(tests_dir, kphp_path, jobs, test_tags, no_report, passed_list, test_list, distcc_hosts):
    hack_reference_exit = []
    signal.signal(signal.SIGINT, lambda sig, frame: hack_reference_exit.append(1))

    tests = collect_tests(tests_dir, test_tags, test_list)
    if not tests:
        print("Can't find any tests with [{}] {}".format(
            ", ".join(test_tags),
            "tag" if len(test_tags) == 1 else "tags"))
        sys.exit(1)

    results = []
    with ThreadPool(jobs) as pool:
        tests_completed = 0
        for test_result in pool.imap_unordered(partial(run_test, kphp_path, distcc_hosts), tests):
            if hack_reference_exit:
                print(yellow("Testing process was interrupted"), flush=True)
                break
            tests_completed = tests_completed + 1
            test_result.print_short_report(len(tests), tests_completed)
            results.append(test_result)

    print("\nTesting results:", flush=True)

    skipped = len(tests) - len(results)
    failed = 0
    passed = []
    for test_result in results:
        if test_result.is_skipped():
            skipped = skipped + 1
        elif test_result.is_failed():
            failed = failed + 1
        else:
            passed.append(os.path.relpath(test_result.test_file_path, tests_dir))

    if passed:
        print("  {}{}".format(green("passed:  "), len(passed)))
        if passed_list:
            with open(passed_list, "w") as f:
                passed.sort()
                f.writelines("{}\n".format(l) for l in passed)
    if skipped:
        print("  {}{}".format(yellow("skipped: "), skipped))
    if failed:
        print("  {}{}\n".format(red("failed:  "), failed))
        if not no_report:
            for test_result in results:
                test_result.print_fail_report()

    sys.exit(1 if failed else len(hack_reference_exit))


def parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "-j",
        type=int,
        dest="jobs",
        default=(multiprocessing.cpu_count() // 2) or 1,
        help="number of parallel jobs")

    this_dir = os.path.dirname(__file__)
    parser.add_argument(
        "-d",
        type=str,
        dest="tests_dir",
        default=os.path.join(this_dir, "phpt"),
        help="tests dir")

    parser.add_argument(
        "--kphp",
        type=str,
        dest="kphp_path",
        default=os.path.normpath(os.path.join(this_dir, os.path.pardir, "kphp.sh")),
        help="path to kphp")

    parser.add_argument(
        'test_tags',
        metavar='TAG',
        type=str,
        nargs='*',
        help='test tag or directory or file')

    parser.add_argument(
        "--no-report",
        action='store_true',
        dest="no_report",
        default=False,
        help="do not show full report")

    parser.add_argument(
        '--save-passed',
        metavar='FILE',
        type=str,
        dest="passed_list",
        default=None,
        help='save passed tests in separate file')

    parser.add_argument(
        '--from-list',
        metavar='FILE',
        type=str,
        dest="test_list",
        default=None,
        help='run tests from list')

    parser.add_argument(
        '--distcc-host-list',
        metavar='FILE',
        type=str,
        dest='distcc_host_list',
        default=None,
        help='list of available distcc hosts')

    return parser.parse_args()


def main():
    args = parse_args()
    if not os.path.exists(args.tests_dir):
        print("Can't find tests dir '{}'".format(args.test_list))
        sys.exit(1)

    if not os.path.exists(args.kphp_path):
        print("Can't find kphp '{}'".format(args.kphp_path))
        sys.exit(1)

    if args.test_list and not os.path.exists(args.test_list):
        print("Can't find test list file '{}'".format(args.test_list))
        sys.exit(1)

    try:
        distcc_hosts = read_distcc_hosts(args.distcc_host_list)
    except Exception as ex:
        print(str(ex))
        sys.exit(1)

    run_all_tests(tests_dir=os.path.normpath(args.tests_dir),
                  kphp_path=os.path.normpath(args.kphp_path),
                  jobs=args.jobs,
                  test_tags=args.test_tags,
                  no_report=args.no_report,
                  passed_list=args.passed_list,
                  test_list=args.test_list,
                  distcc_hosts=distcc_hosts)


if __name__ == "__main__":
    main()
