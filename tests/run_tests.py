#!/usr/bin/env python3
import argparse
import curses
from enum import Enum
import os
import resource
import subprocess
import sys

from termcolor import colored, cprint


class TestStatus(Enum):
    FAILED = colored("failed", "red")
    PASSED = colored("passed", "green")
    SKIPPED = colored("skipped", "yellow")


class TestGroup:
    def __init__(self, name, description, cmd, cwd=None):
        self.name = name
        self.description = description
        self.template_cmd = cmd
        self.status = TestStatus.SKIPPED
        self.args = {}
        self.enabled = False
        self.cwd = cwd

    def setup(self, enabled, args):
        self.enabled = enabled
        self.args = args

    def get_cmd(self):
        return self.template_cmd.format(**self.args)

    def print_full_test_cmd(self):
        cprint(self.get_cmd(), "cyan", flush=True)

    def run(self):
        if not self.enabled:
            return

        cprint("<" * curses.tigetnum("cols"), "yellow", flush=True)
        cprint("Starting {}...".format(self.description), "yellow", flush=True)
        self.print_full_test_cmd()
        test_retcode = subprocess.call(self.get_cmd(), shell=True, cwd=self.cwd)
        if test_retcode:
            self.status = TestStatus.FAILED
            cprint("{} failed, please check.".format(self.description), "red", flush=True)
            cprint(">" * curses.tigetnum("cols"), "red", flush=True)
        else:
            self.status = TestStatus.PASSED
            cprint("{} successfully finished!".format(self.description), "green", flush=True)
            cprint(">" * curses.tigetnum("cols"), "green", flush=True)

    def print_report(self):
        print("\t[{}] {} - {}".format(
            self.name, self.description, self.status.value), flush=True)
        if self.status != TestStatus.PASSED:
            self.print_full_test_cmd()


class TestRunner:
    def __init__(self, report_hint):
        self.test_list = []
        self.no_report = False
        self.report_hint = report_hint

    def add_test_group(self, name, description, cmd, cwd):
        self.test_list.append(TestGroup(name, description, cmd, cwd))

    def setup(self):
        self._parse_args()

        if not self.no_report and any(test.enabled for test in self.test_list):
            self._clear_screen()

        self._setup_coredump_handling()

    def _parse_args(self):
        parser = argparse.ArgumentParser(
            formatter_class=argparse.RawTextHelpFormatter
        )

        parser.add_argument(
            "-j",
            "--jobs",
            type=int,
            dest="jobs",
            default=8,#int(psutil.cpu_count() * 1.5),
            help="Number of jobs to some tests"
        )
        parser.add_argument(
            "--no-report",
            action='store_true',
            dest="no_report",
            default=False,
            help="Do not show full report."
        )
        all_tests = [g.name for g in self.test_list]
        parser.add_argument(
            "-r",
            nargs='*',
            dest="tests",
            default=all_tests,
            help="Group of tests to run: [{}]".format(", ".join(all_tests))
        )
        args = parser.parse_args()

        self.no_report = args.no_report
        for test_name in args.tests:
            if test_name not in all_tests:
                print("Unknown test group passed '{}'".format(test_name), flush=True)
                parser.print_help()
                sys.exit(1)

        for test in self.test_list:
            test.setup(
                enabled=(test.name in args.tests),
                args={"jobs": args.jobs}
            )

    def run_tests(self):
        for test in self.test_list:
            test.run()

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
    def _setup_coredump_handling():
        resource.setrlimit(
            resource.RLIMIT_CORE,
            (resource.RLIM_INFINITY, resource.RLIM_INFINITY))

    @staticmethod
    def _clear_screen():
        screen_lines_count = curses.tigetnum("lines")
        print("\n" * screen_lines_count, flush=True)


if __name__ == "__main__":
    curses.setupterm()

    runner_cwd = os.path.dirname(os.path.abspath(__file__))
    test_runner = os.path.join(runner_cwd, "tester.py")

    runner = TestRunner("KPHP tests")
    runner.add_test_group(
        name="php",
        description="generate php response",
        cmd="{} -A -a ok -f -m php".format(test_runner),
        cwd=runner_cwd
    )
 
    runner.add_test_group(
        name="kphp",
        description="generate kphp response and match with php",
        cmd="{} -a ok -f".format(test_runner),
        cwd=runner_cwd
    )

    runner.add_test_group(
        name="should-failed",
        description="run only kphp",
        cmd="{} -a kphp_should_fail -f".format(test_runner),
        cwd=runner_cwd
    )

    runner.setup()
    runner.run_tests()
    runner.print_report()
    sys.exit(runner.get_exit_code())
