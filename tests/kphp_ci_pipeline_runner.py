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


CC2CXX_MAP = {"gcc": "g++", "clang": "clang++"}


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
        default="",
        help="specify path to zend php-src repository dir"
    )

    parser.add_argument(
        "--engine-repo",
        metavar="DIR",
        type=str,
        dest="engine_repo",
        default="",
        help="specify path to vk engine repository dir"
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
        "--compiler",
        metavar="MODE",
        type=str,
        dest="compiler",
        default="gcc",
        choices=CC2CXX_MAP.keys(),
        help="specify compiler for tests"

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
    functional_tests_dir = make_relpath(runner_dir, "python/tests")
    zend_test_list = make_relpath(runner_dir, "zend-test-list")
    kphp_repo_root = os.path.join(runner_dir, os.path.pardir)
    kphp_repo_root = os.path.relpath(kphp_repo_root, os.getcwd())

    args = parse_args()
    runner = TestRunner("KPHP tests", args.no_report)

    cc_compiler = args.compiler
    cxx_compiler = CC2CXX_MAP[cc_compiler]

    cmake_options = []
    env_vars = []
    if args.use_asan:
        cmake_options.append("-DADDRESS_SANITIZER=ON")
        env_vars.append("ASAN_OPTIONS=detect_leaks=0")
    if args.use_ubsan:
        cmake_options.append("-DUNDEFINED_SANITIZER=ON")
        env_vars.append("UBSAN_OPTIONS=print_stacktrace=1:allow_addr2line=1")
    cmake_options = " ".join(cmake_options)
    env_vars = " ".join(env_vars)

    kphp_polyfills_repo = args.kphp_polyfills_repo
    if kphp_polyfills_repo == "":
        print(red("empty --kphp-polyfills-repo argument"), flush=True)
    kphp_polyfills_repo = os.path.abspath(kphp_polyfills_repo)

    distcc_options = ""
    distcc_cmake_option = ""
    distcc_hosts_file = ""
    if args.use_distcc:
        distributive_name = get_distributive_name()
        distcc_hosts_file = "/etc/distcc/hosts"
        distcc_options = "--distcc-host-list {}".format(distcc_hosts_file)
        os.environ.update(make_distcc_env(read_distcc_hosts(distcc_hosts_file), os.path.join(runner_dir, "tmp_distcc")))
        distcc_cmake_option = "-DCMAKE_CXX_COMPILER_LAUNCHER=distcc "

    runner.add_test_group(
        name="make-kphp",
        description="make kphp and runtime",
        cmd="rm -rf {kphp_repo_root}/build && "
            "mkdir {kphp_repo_root}/build && "
            "cmake "
            "-S {kphp_repo_root} -B {kphp_repo_root}/build "
            "{distcc_cmake_option}"
            "-DCMAKE_CXX_COMPILER={cxx} {cmake_options} && "
            "{env_vars} make -C {kphp_repo_root}/build -j{{jobs}} all test && "
            "{env_vars} make -C {kphp_repo_root}/build vkext7.2 vkext7.4".format(
                kphp_repo_root=kphp_repo_root,
                distcc_cmake_option=distcc_cmake_option,
                cc=cc_compiler,
                cxx=cxx_compiler,
                cmake_options=cmake_options,
                env_vars=env_vars
            ),
        skip=args.steps and "make-kphp" not in args.steps
    )

    if args.engine_repo:
        engine_cc = "distcc gcc" if args.use_distcc else "gcc"
        engine_cxx = "distcc g++" if args.use_distcc else "g++"
        runner.add_test_group(
            name="make-engines",
            description="make engines",
            cmd="{env_vars} PATH=\"{kphp_repo_root}/objs/bin:$PATH\" CC='{engine_cc}' CXX='{engine_cxx}' make -C {engine_repo_root} -j{{jobs}} "
                "objs/bin/combined.tlo objs/bin/combined2.tl tlclient tasks rpc-proxy pmemcached memcached".format(
                    env_vars=env_vars,
                    kphp_repo_root=os.path.abspath(kphp_repo_root),
                    engine_repo_root=args.engine_repo,
                    engine_cc=engine_cc,
                    engine_cxx=engine_cxx
            ),
            skip=args.steps and "make-engines" not in args.steps
        )

    runner.add_test_group(
        name="kphp-tests",
        description="run kphp tests in {} mode".format("gcc"),
        cmd="KPHP_TESTS_POLYFILLS_REPO={kphp_polyfills_repo} "
            "{kphp_runner} -j{{jobs}} {distcc_options}".format(
            kphp_polyfills_repo=kphp_polyfills_repo,
            kphp_runner=kphp_test_runner,
            distcc_options=distcc_options
        ),
        skip=args.steps and "kphp-tests" not in args.steps
    )

    if args.zend_repo:
        runner.add_test_group(
            name="zend-tests",
            description="run php tests from zend repo",
            cmd="{kphp_runner} -j{{jobs}} -d {zend_repo} --from-list {zend_tests} {distcc_options}".format(
                kphp_runner=kphp_test_runner,
                zend_repo=args.zend_repo,
                zend_tests=zend_test_list,
                distcc_options=distcc_options
            ),
            skip=args.steps and "zend-tests" not in args.steps
        )

    tl2php_bin = os.path.join(kphp_repo_root, "objs/bin/tl2php")
    combined_tlo = os.path.join(args.engine_repo or kphp_repo_root, "objs/bin/combined.tlo")
    combined2_tl = os.path.join(args.engine_repo or kphp_repo_root, "objs/bin/combined2.tl")
    tl_tests_dir = make_relpath(runner_dir, "TL-tests")
    runner.add_test_group(
        name="tl2php",
        description="gen php classes with tests from tl schema in {} mode".format("gcc"),
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
        description="run typed tl tests in {} mode".format("gcc"),
        cmd="KPHP_TESTS_POLYFILLS_REPO={kphp_polyfills_repo} "
            "KPHP_TL_SCHEMA={combined_tlo} "
            "KPHP_GEN_TL_INTERNALS=1 "
            "{kphp_runner} -j{{jobs}} -d {tl_tests_dir} {distcc_options}".format(
            kphp_polyfills_repo=kphp_polyfills_repo,
            combined_tlo=os.path.abspath(combined_tlo),
            kphp_runner=kphp_test_runner,
            tl_tests_dir=tl_tests_dir,
            distcc_options=distcc_options
        ),
        skip=args.steps and "typed-tl-tests" not in args.steps
    )

    runner.add_test_group(
        name="functional-tests",
        description="run kphp functional tests in {} mode".format("gcc"),
        cmd="KPHP_TESTS_DISTCC_FILE={distcc_hosts_file} "
            "python3 -m pytest --tb=native -n{{jobs}} {functional_tests_dir}".format(
            functional_tests_dir=functional_tests_dir,
            distcc_hosts_file=distcc_hosts_file
        ),
        skip=args.steps and "functional-tests" not in args.steps
    )

    if args.engine_repo and args.kphp_tests_repo:
        runner.add_test_group(
            name="integration-tests",
            description="run kphp integration tests in {} mode".format("gcc"),
            cmd="PYTHONPATH={lib_dir} "
                "KPHP_TESTS_ENGINE_REPO={engine_repo} "
                "KPHP_TESTS_KPHP_REPO={kphp_repo_root} "
                "KPHP_TESTS_DISTCC_FILE={distcc_hosts_file} "
                "python3 -m pytest --tb=native -n{{jobs}} {tests_dir}".format(
                lib_dir=os.path.join(runner_dir, "python"),
                engine_repo=args.engine_repo,
                kphp_repo_root=kphp_repo_root,
                distcc_hosts_file=distcc_hosts_file,
                tests_dir=os.path.join(args.kphp_tests_repo, "python/tests"),
            ),
            skip=args.steps and "integration-tests" not in args.steps
        )

    runner.setup(args)
    runner.run_tests()
    runner.print_report()
    sys.exit(runner.get_exit_code())
