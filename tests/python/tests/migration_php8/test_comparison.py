import os
from typing import List

from python.lib.testcase import KphpCompilerAutoTestCase

from tests.python.lib.colors import green, red, yellow


class TestPHP8NumberStringComparison(KphpCompilerAutoTestCase):
    def test_comparison(self):
        self.run_for_file("php/array_func.php")
        self.run_for_file("php/equal.php")
        self.run_for_file("php/enable_disable.php")

    def run_for_file(self, file: str):
        expected_warnings: List[str] = []

        script_path = os.path.join(self.test_dir, file)
        with open(script_path, "r") as script_file:
            line = script_file.readline()
            while "<?" not in line:
                line = line.replace("\n", "")
                line = line.strip()
                if line != "":
                    expected_warnings.append(line)
                line = script_file.readline()

        once_runner = self.make_kphp_once_runner(file)
        self.assertTrue(once_runner.compile_with_kphp())
        self.assertTrue(once_runner.run_with_kphp())

        got_warnings: List[str] = []

        if len(once_runner.artifacts) != 0:
            runtime_artifact = once_runner.artifacts["kphp runtime stderr"]

            runtime_lines: List[str] = []
            with open(runtime_artifact.file, "r") as error_file:
                runtime_lines = error_file.readlines()

            for i in range(len(runtime_lines)):
                if "Stack Backtrace" in runtime_lines[i] and i > 0:
                    warning_line = runtime_lines[i - 1]
                    warning_line = warning_line.replace("\n", "")
                    warning_line = warning_line.strip()
                    if warning_line == "":
                        continue

                    warning_line = warning_line[warning_line.index("Warning: ") + len("Warning: "):]
                    got_warnings.append(warning_line)

        matched = self.compare_warnings(got_warnings, expected_warnings)

        if not matched:
            print(red("Error in {} test\n".format(file)))
        else:
            print(green("Test {} passed\n".format(file)))

        self.assertTrue(matched)

    @staticmethod
    def compare_warnings(got: List[str], expected: List[str]):
        matched = True
        for index in range(min(len(got), len(expected))):
            if expected[index] not in got[index]:
                print("Can't find {}th error pattern: \nexpected: \n   {}\ngot: \n   {}".format(index, green(expected[index]), yellow(got[index])))
                matched = False

        if len(got) > len(expected):
            matched = False
            print("Unhandled warnings:")
            for i in range(len(expected), len(got)):
                print(yellow("  " + got[i]))

        if len(expected) > len(got):
            matched = False
            print("Extra warnings:")
            for i in range(len(got), len(expected)):
                print(yellow("  " + expected[i]))

        return matched
