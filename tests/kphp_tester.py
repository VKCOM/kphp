#!/usr/bin/python3
import argparse
import math
import multiprocessing
import os
import re
import signal
import sys
from functools import partial
from multiprocessing.dummy import Pool as ThreadPool

from python.lib.colors import red, green, yellow, blue
from python.lib.file_utils import search_php_bin
from python.lib.nocc_for_kphp_tester import nocc_start_daemon_in_background
from python.lib.kphp_run_once import KphpRunOnce


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

    def is_idempotent(self):
        return "non-idempotent" not in self.tags

    def is_kphp_should_fail(self):
        return "kphp_should_fail" in self.tags

    def is_kphp_should_warn(self):
        return "kphp_should_warn" in self.tags

    def is_kphp_runtime_should_warn(self):
        return "kphp_runtime_should_warn" in self.tags

    def is_kphp_runtime_should_not_warn(self):
        return "kphp_runtime_should_not_warn" in self.tags

    def is_php8(self):
        return "php8" in self.tags

    def make_kphp_once_runner(self, use_nocc, cxx_name):
        tester_dir = os.path.abspath(os.path.dirname(__file__))
        return KphpRunOnce(
            php_script_path=self.file_path,
            working_dir=os.path.abspath(os.path.join(self.test_tmp_dir, "working_dir")),
            artifacts_dir=os.path.abspath(os.path.join(self.test_tmp_dir, "artifacts")),
            php_bin=search_php_bin(php8_require=self.is_php8()),
            extra_include_dirs=[os.path.join(tester_dir, "php_include")],
            vkext_dir=os.path.abspath(os.path.join(tester_dir, os.path.pardir, "objs", "vkext")),
            use_nocc=use_nocc,
            cxx_name=cxx_name,
        )


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

        forbidden_regexps = []
        out_regexps = []
        env_vars = {}  # string -> string, e.g. {"KPHP_REQUIRE_FUNCTIONS_TYPING" -> "1"}
        while True:
            second_line = f.readline().decode('utf-8').strip()
            if len(second_line) > 1 and second_line.startswith("/") and second_line.endswith("/"):
                out_regexps.append(re.compile(second_line[1:-1]))
            elif len(second_line) > 2 and second_line.startswith("!/") and second_line.endswith("/"):
                forbidden_regexps.append(re.compile(second_line[2:-1]))
            elif second_line.startswith("KPHP_") and "=" in second_line:
                env_name, env_value = parse_env_var_from_test_header(file_path, second_line)
                env_vars[env_name] = env_value
            else:  # <?php
                break

        return TestFile(file_path, test_tmp_dir, tags, env_vars, out_regexps, forbidden_regexps)


def parse_env_var_from_test_header(file_path, line):
    env_name, env_value = line.split('=', 2)
    # placeholders in file header, e.g. KPHP_COMPOSER_ROOT={dir}
    if "{" in env_value:
        env_value = env_value.replace("{dir}", os.path.dirname(os.path.abspath(file_path)))
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

    if not runner.kphp_build_stderr_artifact:
        return TestResult.failed(test, runner.artifacts, "kphp build failed without stderr")

    if test.out_regexps or test.forbidden_regexps:
        with open(runner.kphp_build_stderr_artifact.file) as f:
            stderr_log = f.read()
            for index, msg_regex in enumerate(test.out_regexps, start=1):
                if not msg_regex.search(stderr_log):
                    return TestResult.failed(test, runner.artifacts, "can't find {}th error pattern".format(index))

            for index, forbidden_regex in enumerate(test.forbidden_regexps, start=1):
                if forbidden_regex.search(stderr_log):
                    return TestResult.failed(test, runner.artifacts, "{}th forbidden error pattern is found".format(index))

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

        for index, forbidden_regex in enumerate(test.forbidden_regexps, start=1):
            if forbidden_regex.search(stderr_log):
                return TestResult.failed(test, runner.artifacts, "{}th forbidden warning pattern is found".format(index))

    if runner.kphp_build_sanitizer_log_artifact:
        return TestResult.failed(test, runner.artifacts, "got sanitizer log")

    runner.kphp_build_stderr_artifact.error_priority = -1
    return TestResult.passed(test, runner.artifacts)

def run_runtime_not_warn_test(test: TestFile, runner):
    if not runner.compile_with_kphp(test.env_vars):
        return TestResult.failed(test, runner.artifacts, "got kphp build error")

    if not runner.run_with_kphp():
        return TestResult.failed(test, runner.artifacts, "got kphp run error")

    if runner.kphp_runtime_stderr is not None:
        return TestResult.failed(test, runner.artifacts, "got kphp runtime error")

    if runner.kphp_build_sanitizer_log_artifact:
        return TestResult.failed(test, runner.artifacts, "got sanitizer log")

    return TestResult.passed(test, runner.artifacts)

def run_runtime_warn_test(test: TestFile, runner):
    if not runner.compile_with_kphp(test.env_vars):
        return TestResult.failed(test, runner.artifacts, "got kphp build error")

    if not runner.run_with_kphp():
        return TestResult.failed(test, runner.artifacts, "got kphp run error")

    with open(runner.kphp_runtime_stderr.file) as f:
        stderr_log = f.read()
        if not re.search("Warning: ", stderr_log):
            return TestResult.failed(test, runner.artifacts, "can't find kphp runtime warnings")
        for index, msg_regex in enumerate(test.out_regexps, start=1):
            if not msg_regex.search(stderr_log):
                return TestResult.failed(test, runner.artifacts, "can't find {}th runtime warning pattern".format(index))

        for index, forbidden_regex in enumerate(test.forbidden_regexps, start=1):
            if forbidden_regex.search(stderr_log):
                return TestResult.failed(test, runner.artifacts, "{}th forbidden runtime warning pattern is found".format(index))

    if runner.kphp_build_sanitizer_log_artifact:
        return TestResult.failed(test, runner.artifacts, "got sanitizer log")

    return TestResult.passed(test, runner.artifacts)


def run_ok_test(test: TestFile, runner):
    # Run kphp test twice to check correctness in web-server alike environment with per request runtime reinitialization
    runs_cnt = 2 if test.is_idempotent() else 1
    if not runner.run_with_php(runs_cnt=runs_cnt):
        return TestResult.failed(test, runner.artifacts, "got php error")
    if not runner.compile_with_kphp(test.env_vars):
        return TestResult.failed(test, runner.artifacts, "got kphp build error")
    if not runner.run_with_kphp(runs_cnt=runs_cnt):
        return TestResult.failed(test, runner.artifacts, "got kphp runtime error")
    if not runner.compare_php_and_kphp_stdout():
        return TestResult.failed(test, runner.artifacts, "got php and kphp diff")

    return TestResult.passed(test, runner.artifacts)


def run_test(use_nocc, cxx_name, test: TestFile):
    if not os.path.exists(test.file_path):
        return TestResult.failed(test, None, "can't find test file")

    runner = test.make_kphp_once_runner(use_nocc, cxx_name)
    runner.remove_artifacts_dir()

    if test.is_php8() and runner._php_bin is None:      # if php8 doesn't exist on a machine
        test_result = TestResult.skipped(test)
    elif test.is_kphp_should_fail():
        test_result = run_fail_test(test, runner)
    elif test.is_kphp_should_warn():
        test_result = run_warn_test(test, runner)
    elif test.is_kphp_runtime_should_warn():
        test_result = run_runtime_warn_test(test, runner)
    elif test.is_kphp_runtime_should_not_warn():
        test_result = run_runtime_not_warn_test(test, runner)
    elif test.is_ok():
        test_result = run_ok_test(test, runner)
    else:
        test_result = TestResult.skipped(test)

    runner.try_remove_kphp_build_trash()

    return test_result


def run_all_tests(tests_dir, jobs, test_tags, no_report, passed_list, test_list, use_nocc, cxx_name):
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
        for test_result in pool.imap_unordered(partial(run_test, use_nocc, cxx_name), tests):
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
        default=multiprocessing.cpu_count(),
        help="number of parallel jobs")

    this_dir = os.path.dirname(__file__)
    parser.add_argument(
        "-d",
        type=str,
        dest="tests_dir",
        default=os.path.join(this_dir, "phpt"),
        help="tests dir")

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
        '--use-nocc',
        action='store_true',
        dest='use_nocc',
        default=False,
        help='use nocc in KPHP_CXX; if true, env NOCC_SERVERS must be set')

    parser.add_argument(
        "--cxx-name",
        type=str,
        dest="cxx_name",
        default="g++",
        help="specify cxx compiler, default g++")

    return parser.parse_args()


def main():
    args = parse_args()
    if not os.path.exists(args.tests_dir):
        print("Can't find tests dir '{}'".format(args.test_list))
        sys.exit(1)

    if args.test_list and not os.path.exists(args.test_list):
        print("Can't find test list file '{}'".format(args.test_list))
        sys.exit(1)

    try:
        if args.use_nocc:
            # see a comment above this function why we are doing it manually
            nocc_start_daemon_in_background()
    except Exception as ex:
        print(str(ex))
        sys.exit(1)

    run_all_tests(tests_dir=os.path.normpath(args.tests_dir),
                  jobs=args.jobs,
                  test_tags=args.test_tags,
                  no_report=args.no_report,
                  passed_list=args.passed_list,
                  test_list=args.test_list,
                  use_nocc=args.use_nocc,
                  cxx_name=args.cxx_name)


if __name__ == "__main__":
    main()
