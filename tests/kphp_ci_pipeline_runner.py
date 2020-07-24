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
from python.lib.file_utils import make_distcc_env, read_distcc_hosts


def get_distributive_name():
    with open("/etc/issue") as f:
        version_line = f.readline()
    if version_line.startswith("Debian GNU/Linux 10 "):
        return "buster"
    if version_line.startswith("Debian GNU/Linux 8 "):
        return "jessie"
    return "unknown"


class TestStatus(Enum):
    FAILED = red("failed")
    PASSED = green("passed")
    SKIPPED = yellow("skipped")


class TestGroup:
    def __init__(self, name, description, cmd, skip):
        self.name = name
        self.description = description
        self.template_cmd = cmd
        self.status = TestStatus.SKIPPED
        self.params = {}
        self.skip = skip

    def setup(self, params):
        self.params = params

    def get_cmd(self):
        return self.template_cmd.format(**self.params)

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

        for test in self.test_list:
            test.setup(params={"jobs": params.jobs})

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


MAKE_TARGETS = "php tl2php kphp-unittests tlclient tasks rpc-proxy"
TESTING_MODES = {
    "gcc": ("", "gcc", "g++", MAKE_TARGETS),
    "gcc-asan": (
        "ASAN_OPTIONS=detect_leaks=0", "gcc", "g++",
        "g=1 asan=1 " + MAKE_TARGETS
    ),
    "clang-ubsan": (
        "UBSAN_OPTIONS=print_stacktrace=1:allow_addr2line=1", "clang", "clang++",
        "g=1 ubsan=1 " + MAKE_TARGETS
    )
}

# TODO: Remove this code after teamcity update
TESTING_MODES["normal"] = TESTING_MODES["gcc"]
TESTING_MODES["asan"] = TESTING_MODES["gcc-asan"]


def parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        dest="jobs",
        default=multiprocessing.cpu_count(),
        help="number of jobs to some tests"
    )

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
        default=os.path.expanduser("~/php-src"),
        help="specify path to zend php-src repository dir"
    )

    parser.add_argument(
        "--mode",
        metavar="MODE",
        type=str,
        dest="mode",
        default="asan",
        choices=TESTING_MODES.keys(),
        help="specify testing mode, allowed: {}".format(", ".join(TESTING_MODES.keys()))
    )

    parser.add_argument(
        "--use-distcc",
        action='store_true',
        dest="use_distcc",
        default=False,
        help="use distcc for building kphp tests"
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
    functional_tests_dir = make_relpath(runner_dir, "python/t")
    zend_test_list = make_relpath(runner_dir, "ci/zend-test-list")
    root_engine_path = os.path.join(os.path.join(runner_dir, os.path.pardir), os.path.pardir)
    root_engine_path = os.path.relpath(root_engine_path, os.getcwd())

    args = parse_args()
    runner = TestRunner("KPHP tests", args.no_report)

    env_vars, cc, cxx, targets = TESTING_MODES[args.mode]
    distcc_options = ""
    distcc_host_list = ""
    distributive_name = get_distributive_name()
    if args.use_distcc or distributive_name != "unknown":
        distcc_hosts = "ci/distcc-host-list.{}".format(distributive_name)
        distcc_host_list = make_relpath(runner_dir, distcc_hosts)
        distcc_options = "--distcc-host-list {}".format(distcc_host_list)
        os.environ.update(make_distcc_env(read_distcc_hosts(distcc_host_list), os.path.join(runner_dir, "tmp_distcc")))
        if "KDB_USE_CCACHE" not in os.environ:
            cc = "'distcc {}'".format(cc)
            cxx = "'distcc {}'".format(cxx)

    runner.add_test_group(
        name="make-kphp",
        description="make kphp and runtime in {} mode".format(args.mode),
        cmd="CC={cc} CXX={cxx} {env_vars} make -C {engine_root} -j{{jobs}} {targets}".format(
            cc=cc,
            cxx=cxx,
            env_vars=env_vars,
            engine_root=root_engine_path,
            targets=targets
        ),
        skip=args.steps and "make-kphp" not in args.steps
    )

    runner.add_test_group(
        name="kphp-tests",
        description="run kphp tests in {} mode".format(args.mode),
        cmd="{kphp_runner} -j{{jobs}} {distcc_options}".format(
            kphp_runner=kphp_test_runner,
            distcc_options=distcc_options
        ),
        skip=args.steps and "kphp-tests" not in args.steps
    )

    skip_zend_tests = not os.path.exists(args.zend_repo)
    if "zend-tests" in args.steps and skip_zend_tests:
        print("Trying to use zend tests, but there is no zend repo directory", flush=True)
        sys.exit(1)
    elif args.steps and "zend-tests" not in args.steps:
        skip_zend_tests = True

    runner.add_test_group(
        name="zend-tests",
        description="run php tests from zend repo in {} mode".format(args.mode),
        cmd="{kphp_runner} -j{{jobs}} -d {zend_repo} --from-list {zend_tests} {distcc_options}".format(
            kphp_runner=kphp_test_runner,
            zend_repo=args.zend_repo,
            zend_tests=zend_test_list,
            distcc_options=distcc_options
        ),
        skip=skip_zend_tests
    )

    tl2php_bin = os.path.join(root_engine_path, "objs/bin/tl2php")
    combined_tlo = os.path.join(root_engine_path, "objs/bin/combined.tlo")
    combined2_tl = os.path.join(root_engine_path, "objs/bin/combined2.tl")
    tl_tests_dir = make_relpath(runner_dir, "TL-tests")
    runner.add_test_group(
        name="tl2php",
        description="gen php classes with tests from tl schema in {} mode".format(args.mode),
        cmd="{env_vars} {tl2php} -i -c {combined2_tl} -t -f -d {tl_tests_dir} {combined_tlo}".format(
            env_vars=env_vars,
            tl2php=tl2php_bin,
            combined2_tl=combined2_tl,
            tl_tests_dir=tl_tests_dir,
            combined_tlo=combined_tlo
        ),
        skip=args.steps and "tl2php" not in args.steps
    )

    runner.add_test_group(
        name="typed-tl-tests",
        description="run typed tl tests in {} mode".format(args.mode),
        cmd="KPHP_TL_SCHEMA={combined_tlo} KPHP_GEN_TL_INTERNALS=1 {kphp_runner} -j{{jobs}} -d {tl_tests_dir} {distcc_options}".format(
            combined_tlo=os.path.abspath(combined_tlo),
            kphp_runner=kphp_test_runner,
            tl_tests_dir=tl_tests_dir,
            distcc_options=distcc_options
        ),
        skip=args.steps and "typed-tl-tests" not in args.steps
    )

    if args.mode != "asan" or get_distributive_name() != "jessie":
        distcc_hosts_env_var = ""
        if distcc_host_list:
            distcc_hosts_env_var = "KPHP_TESTS_DISTCC_FILE={} ".format(os.path.abspath(distcc_host_list))
        runner.add_test_group(
            name="functional-tests",
            description="run kphp functional tests in {} mode".format(args.mode),
            cmd="{distcc_hosts_env_var}python3 -m pytest --tb=native -n{{jobs}} {functional_tests_dir}".format(
                functional_tests_dir=functional_tests_dir,
                distcc_hosts_env_var=distcc_hosts_env_var
            ),
            skip=args.steps and "functional-tests" not in args.steps
        )

    runner.setup(args)
    runner.run_tests()
    runner.print_report()
    sys.exit(runner.get_exit_code())
