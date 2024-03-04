#!/usr/bin/env python3
import argparse
import curses
import multiprocessing
import os
import signal
import subprocess
import sys
from enum import Enum

from python.lib.colors import red, green, yellow, cyan
from python.lib.nocc_for_kphp_tester import nocc_env


class TestStatus(Enum):
    FAILED = red("failed")
    PASSED = green("passed")
    SKIPPED = yellow("skipped")


class TestGroup:
    def __init__(self, name, description, cmd, skip):
        self.name = name
        self.description = description
        self.cmd = cmd
        self.status = TestStatus.SKIPPED
        self.skip = skip

    def get_cmd(self):
        return self.cmd

    def print_full_test_cmd(self):
        print(cyan(self.get_cmd()), flush=True)

    def run(self):
        if self.skip:
            return True

        print(yellow("<" * curses.tigetnum("cols")), flush=True)
        print(yellow("Starting {}...".format(self.description)), flush=True)
        self.print_full_test_cmd()
        test_retcode = subprocess.call(self.get_cmd(), shell=True)
        if test_retcode:
            self.status = TestStatus.FAILED
            print(red("{} failed, please check.".format(self.description)), flush=True)
            print(red(">" * curses.tigetnum("cols")), flush=True)
        else:
            self.status = TestStatus.PASSED
            print(green("{} successfully finished!".format(self.description)), flush=True)
            print(green(">" * curses.tigetnum("cols")), flush=True)

        return not test_retcode

    def print_report(self):
        print("\t[{}] {} - {}".format(
            self.name, self.description, self.status.value), flush=True)
        if self.status != TestStatus.PASSED:
            self.print_full_test_cmd()


class TestRunner:
    def __init__(self, report_hint, no_report):
        self.test_list = []
        self.no_report = no_report
        self.report_hint = report_hint
        self.show_stages = False

    def add_test_group(self, name, description, cmd, skip=False):
        self.test_list.append(TestGroup(name, description, cmd, skip))

    def setup(self, params):
        self.show_stages = params.show_stages
        if not self.no_report and not self.show_stages:
            self._clear_screen()

    def run_tests(self):
        if self.show_stages:
            return
        for test in self.test_list:
            if not test.run():
                return

    def print_report(self):
        if self.no_report:
            return

        print("=" * curses.tigetnum("cols"), flush=True)
        print("Tests report ({}):".format(self.report_hint), flush=True)
        for test in self.test_list:
            test.print_report()
        print("=" * curses.tigetnum("cols"), flush=True)

    def get_exit_code(self):
        if any(test.status == TestStatus.FAILED for test in self.test_list):
            return 1
        return 0

    @staticmethod
    def _clear_screen():
        screen_lines_count = curses.tigetnum("lines")
        print("\n" * screen_lines_count, flush=True)


def make_relpath(file_dir, file_path):
    rel_path = os.path.relpath(os.path.join(file_dir, file_path), os.getcwd())
    if rel_path[0] != '/':
        rel_path = os.path.join(os.path.curdir, rel_path)
    return rel_path


def parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        "--no-report",
        action='store_true',
        dest="no_report",
        default=False,
        help="no not show full report"
    )

    parser.add_argument(
        "-s",
        "--show-stages",
        action='store_true',
        dest="show_stages",
        default=False,
        help="show testing stages"
    )

    parser.add_argument(
        "--zend-repo",
        metavar="DIR",
        type=str,
        dest="zend_repo",
        default="",
        help="specify path to zend php-src repository dir"
    )

    parser.add_argument(
        "--engine-repo",
        metavar="DIR",
        type=str,
        dest="engine_repo",
        default="",
        help="Any nonempty value of this option will identify that integration tests are enabled."  # TODO: rename it
    )

    parser.add_argument(
        "--kphp-tests-repo",
        metavar="DIR",
        type=str,
        dest="kphp_tests_repo",
        default="",
        help="specify path to kphp tests repository dir"
    )

    parser.add_argument(
        '--kphp-polyfills-repo',
        metavar="DIR",
        type=str,
        dest="kphp_polyfills_repo",
        default=os.environ.get("KPHP_TESTS_POLYFILLS_REPO", ""),
        help="specify path to cloned kphp-polyfills repository dir"
    )

    parser.add_argument(
        "--cxx-name",
        metavar="MODE",
        type=str,
        dest="cxx_name",
        default="g++",
        choices=["g++", "clang++", "clang++-16"],
        help="specify cxx for compiling kphp and running tests"
    )

    parser.add_argument(
        "--use-asan",
        action='store_true',
        dest="use_asan",
        default=False,
        help="enable address sanitizer for tests"
    )

    parser.add_argument(
        "--use-ubsan",
        action='store_true',
        dest="use_ubsan",
        default=False,
        help="enable undefined behaviour sanitizer for tests"
    )

    parser.add_argument(
        "--use-nocc",
        action='store_true',
        dest="use_nocc",
        default=False,
        help="use nocc for compiling kphp and running phpt"
    )

    parser.add_argument(
        'steps',
        metavar='STEPS',
        type=str,
        nargs='*',
        help='testing steps for ci pipeline')

    return parser.parse_args()


if __name__ == "__main__":
    curses.setupterm()

    signal.signal(signal.SIGINT, lambda sig, frame: None)

    runner_dir = os.path.dirname(os.path.abspath(__file__))
    kphp_test_runner = make_relpath(runner_dir, "kphp_tester.py")
    functional_tests_dir = make_relpath(runner_dir, "python/tests")
    zend_test_list = make_relpath(runner_dir, "zend-test-list")
    kphp_repo_root = os.path.join(runner_dir, os.path.pardir)
    kphp_repo_root = os.path.relpath(kphp_repo_root, os.getcwd())

    args = parse_args()
    runner = TestRunner("KPHP tests", args.no_report)

    use_nocc_option = "--use-nocc" if args.use_nocc else ""
    n_cpu = multiprocessing.cpu_count()

    cmake_options = []
    env_vars = []
    if args.use_asan:
        cmake_options.append("-DADDRESS_SANITIZER=ON")
        env_vars.append("ASAN_OPTIONS=detect_leaks=0")
    if args.use_ubsan:
        cmake_options.append("-DUNDEFINED_SANITIZER=ON")
        env_vars.append("UBSAN_OPTIONS=print_stacktrace=1:allow_addr2line=1")
    if args.use_nocc:
        cmake_options.append("-DCMAKE_CXX_COMPILER_LAUNCHER={}".format(nocc_env("NOCC_EXECUTABLE", "nocc")))
    cmake_options.append("-DPDO_DRIVER_MYSQL=ON")
    cmake_options.append("-DPDO_DRIVER_PGSQL=ON")
    cmake_options.append("-DPDO_LIBS_STATIC_LINKING=ON")
    kphp_polyfills_repo = args.kphp_polyfills_repo
    if kphp_polyfills_repo == "":
        print(red("empty --kphp-polyfills-repo argument"), flush=True)
    kphp_polyfills_repo = os.path.abspath(kphp_polyfills_repo)

    cmake_options = " ".join(cmake_options)
    env_vars = " ".join(env_vars)

    runner.add_test_group(
        name="make-kphp",
        description="make kphp and runtime",
        cmd="rm -rf {kphp_repo_root}/build && "
            "mkdir {kphp_repo_root}/build && "
            "cmake "
            "-S {kphp_repo_root} -B {kphp_repo_root}/build "
            "-DCMAKE_CXX_COMPILER={cxx_name} {cmake_options} && "
            "{env_vars} make -C {kphp_repo_root}/build -j{jobs} all test && "
            "{env_vars} make -C {kphp_repo_root}/build vkext7.4 && "
            "cd {kphp_repo_root}/build && cpack".format(
                jobs=n_cpu * 4 if args.use_nocc else n_cpu,
                kphp_repo_root=kphp_repo_root,
                cxx_name=args.cxx_name,
                cmake_options=cmake_options,
                env_vars=env_vars,
            ),
        skip=args.steps and "make-kphp" not in args.steps
    )

    runner.add_test_group(
        name="kphp-tests",
        description="run kphp tests with cxx={}".format(args.cxx_name),
        cmd="KPHP_TESTS_POLYFILLS_REPO={kphp_polyfills_repo} "
            "{kphp_runner} -j{jobs} --cxx-name {cxx_name} {use_nocc_option}".format(
            jobs=n_cpu,
            kphp_polyfills_repo=kphp_polyfills_repo,
            kphp_runner=kphp_test_runner,
            cxx_name=args.cxx_name,
            use_nocc_option=use_nocc_option,
        ),
        skip=args.steps and "kphp-tests" not in args.steps
    )

    if args.zend_repo:
        runner.add_test_group(
            name="zend-tests",
            description="run php tests from zend repo",
            cmd="{kphp_runner} -j{jobs} -d {zend_repo} --from-list {zend_tests} --cxx-name {cxx_name} {use_nocc_option}".format(
                jobs=n_cpu,
                kphp_runner=kphp_test_runner,
                zend_repo=args.zend_repo,
                zend_tests=zend_test_list,
                cxx_name=args.cxx_name,
                use_nocc_option=use_nocc_option,
            ),
            skip=args.steps and "zend-tests" not in args.steps
        )

    tl2php_bin = os.path.join(kphp_repo_root, "objs/bin/tl2php")
    combined_tlo = os.path.abspath("/usr/share/engine/combined.tlo")
    tl_tests_dir = make_relpath(runner_dir, "TL-tests")
    runner.add_test_group(
        name="tl2php",
        description="gen php classes with tests from tl schema in {} mode".format("gcc"),
        cmd="{env_vars} {tl2php} -i -t -f -d {tl_tests_dir} {combined_tlo}".format(
            env_vars=env_vars,
            tl2php=tl2php_bin,
            tl_tests_dir=tl_tests_dir,
            combined_tlo=combined_tlo
        ),
        skip=args.steps and "tl2php" not in args.steps
    )

    runner.add_test_group(
        name="typed-tl-tests",
        description="run typed tl tests with cxx={}".format(args.cxx_name),
        cmd="KPHP_TESTS_POLYFILLS_REPO={kphp_polyfills_repo} "
            "KPHP_TL_SCHEMA={combined_tlo} "
            "KPHP_GEN_TL_INTERNALS=1 "
            "{kphp_runner} -j{jobs} -d {tl_tests_dir} --cxx-name {cxx_name} {use_nocc_option}".format(
            jobs=n_cpu,
            kphp_polyfills_repo=kphp_polyfills_repo,
            combined_tlo=os.path.abspath(combined_tlo),
            kphp_runner=kphp_test_runner,
            tl_tests_dir=tl_tests_dir,
            cxx_name=args.cxx_name,
            use_nocc_option=use_nocc_option,
        ),
        skip=args.steps and "typed-tl-tests" not in args.steps
    )

    runner.add_test_group(
        name="functional-tests",
        description="run kphp functional tests with cxx={}".format("gcc"),
        cmd="KPHP_TESTS_POLYFILLS_REPO={kphp_polyfills_repo} python3 -m pytest --basetemp={base_tempdir} --tb=native -n{jobs} {functional_tests_dir}".format(
            kphp_polyfills_repo=kphp_polyfills_repo,
            jobs=n_cpu,
            functional_tests_dir=functional_tests_dir,
            base_tempdir=os.path.expanduser('~/_tmp')   # Workaround to make unix socket paths needed by pytest-mysql have length < 108 symbols
                                                        # 108 is Linux limit for some reason, see https://blog.8-p.info/en/2020/06/11/unix-domain-socket-length/
            # nocc will be automatically used if NOCC_SERVERS_FILENAME is set
        ),
        skip=args.steps and "functional-tests" not in args.steps
    )

    if args.engine_repo and args.kphp_tests_repo:
        runner.add_test_group(
            name="integration-tests",
            description="run kphp integration tests with cxx={}".format("gcc"),
            cmd="PYTHONPATH={lib_dir} "
                "KPHP_TESTS_KPHP_REPO={kphp_repo_root} "
                "KPHP_TESTS_POLYFILLS_REPO={kphp_polyfills_repo} "
                "KPHP_TESTS_INTERGRATION_TESTS_ENABLED=1 "
                "python3 -m pytest --tb=native -n{jobs} -k '{exclude_pattern}' {tests_dir}".format(
                jobs=n_cpu,
                lib_dir=os.path.join(runner_dir, "python"),
                engine_repo=args.engine_repo,
                kphp_repo_root=kphp_repo_root,
                kphp_polyfills_repo=kphp_polyfills_repo,
                exclude_pattern="not test_load_and_kill_worker" if args.use_asan else "",   # TODO: ASAN behaves very strange on this test that makes it flaky
                tests_dir=os.path.join(args.kphp_tests_repo, "python/tests"),
                # nocc will be automatically used if NOCC_SERVERS_FILENAME is set
            ),
            skip=args.steps and "integration-tests" not in args.steps
        )

    runner.setup(args)
    runner.run_tests()
    runner.print_report()
    sys.exit(runner.get_exit_code())
