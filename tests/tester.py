#!/usr/bin/python3
import shutil
import sys
import os
import itertools
import argparse
import subprocess
import functools
import signal
import concurrent.futures
import threading
from subprocess import CalledProcessError
from collections import namedtuple
from pathlib import Path

PATH_TO_ENGINE = Path.cwd().parents[1]
PATH_TO_PHP = PATH_TO_ENGINE / "PHP"
DIRECTORY_WITH_TESTS = PATH_TO_PHP / "tests" / "phpt"
SUPPORTED_MODES = ["php", "kphp", "hhvm", "none"]
ANSWERS_PATH = Path("answers")
TMP_PATH = Path("tmp")
PERF_PATH = Path("perf")
dead = False


def init_paths(path_to_engine):
    global PATH_TO_ENGINE
    global PATH_TO_PHP
    global DIRECTORY_WITH_TESTS

    PATH_TO_ENGINE = Path(path_to_engine).resolve()
    PATH_TO_PHP = PATH_TO_ENGINE / "PHP"
    DIRECTORY_WITH_TESTS = PATH_TO_PHP / "tests" / "phpt"


class BashColors:
    HEADER = '\033[95m'
    OK_BLUE = '\033[94m'
    OK_GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    END_COLOR = '\033[0m'

    @staticmethod
    def disable():
        BashColors.HEADER = ''
        BashColors.OK_BLUE = ''
        BashColors.OK_GREEN = ''
        BashColors.WARNING = ''
        BashColors.FAIL = ''
        BashColors.END_COLOR = ''


def get_perf_from_path(path):
    Perf = namedtuple('Perf', ['time', 'memory'])

    if path.exists():
        with path.open() as f:
            perf = f.read().split()
            return Perf(*perf)
    return None


def my_check_call(cmd):
    subprocess.check_call(cmd, shell=True)


def run_make_kphp():
    cmd_make_php = "make -sj30 -C {} php".format(PATH_TO_ENGINE)
    print("run command: '{}'".format(cmd_make_php))
    my_check_call(cmd_make_php)


def add_temp_prefix(name):
    return TMP_PATH / "{thread_id}_{name}".format(thread_id=threading.get_ident(), name=name)


def run_and_save_answer(cmd):
    ans_path = add_temp_prefix("ans")
    perf_path = add_temp_prefix("perf")
    err_path = add_temp_prefix("err")
    dl_time_path = PATH_TO_ENGINE / "objs" / "bin" / "dltime"
    # /usr/bin/time
    full_cmd = "{dl_time} -o {perf_path} {cmd} > {out} 2> {err}".format(
        dl_time=dl_time_path, perf_path=perf_path,
        out=ans_path, err=err_path, cmd=cmd)

    my_check_call(full_cmd)
    return ans_path, perf_path


def run_test_mode_php(path):
    extensions = [
        ("display_errors", 0),
        ("log_errors", 1),
        ("error_log", "/proc/self/fd/2"),
        ("memory_limit", "3072M"),
        ("extension", "json.so"),
        ("extension", "bcmath.so"),
        ("extension", "iconv.so"),
        ("extension", "memcache.so"),
        ("extension", "mbstring.so"),
        ("xdebug.var_display_max_depth", -1),
        ("xdebug.var_display_max_children", -1),
        ("xdebug.var_display_max_data", -1),
        ("include_path", path.parent)
    ]
    extensions_str = " ".join("-d {}='{}'".format(k, v) for (k, v) in extensions)

    cmd = "php -n {extensions} {test}".format(test=path, extensions=extensions_str)

    return run_and_save_answer(cmd)


def run_test_mode_hhvm(path):
    cmd = "hhvm  -v\"Eval.Jit=true\" {}".format(path)
    return run_and_save_answer(cmd)


def get_kphp_out_path():
    return add_temp_prefix("kphp_out")


def run_test_mode_kphp(path):
    path_to_main = add_temp_prefix("main")
    out = get_kphp_out_path()
    path_to_kphp = PATH_TO_PHP / "kphp.sh"

    make_cmd = "{kphp} -S -I {path_to_dir} -M cli " \
               "-o {main} {path} " \
               "> {out} " \
               "2>&1".format(kphp=path_to_kphp, path=path, path_to_dir=path.parent, out=out,
                             main=path_to_main)
    my_check_call(make_cmd)

    return run_and_save_answer(path_to_main)


def get_unique_name_of_test(path):
    path = path.relative_to(DIRECTORY_WITH_TESTS)
    return Path(str(path).replace("/", "@"))


def get_ans_path(path):
    path = get_unique_name_of_test(path).with_suffix(".ans")
    return ANSWERS_PATH / path


def get_perf_path(path, mode):
    path = get_unique_name_of_test(path).with_suffix(".ans")
    return PERF_PATH / mode / path


def perf_cmp(baseline, current, mode):
    baseline = float(baseline._asdict()[mode])
    current = float(current._asdict()[mode])
    eps = 0.01 if mode == "time" else 1000

    baseline = max(baseline, eps)
    current = max(current, eps)
    ratio = baseline / current

    res = "{:.2f}".format(ratio)

    if mode == "time":
        diff = abs(baseline - current)
        ok = (0.9 < ratio < 1.1 and diff < 0.1) or (current < 0.0101 and baseline < 0.0101) or (diff < 0.004)
    else:
        ok = 0.98 < ratio < 1.02

    if ok:
        res = BashColors.OK_BLUE + res + BashColors.END_COLOR
    elif current < baseline:
        res = BashColors.OK_GREEN + res + BashColors.END_COLOR
    else:
        res = BashColors.FAIL + res + BashColors.END_COLOR

    return res


def run_test_mode_helper(mode, test_path):
    run_test_function = {
        "php": run_test_mode_php,
        "kphp": run_test_mode_kphp,
        "hhvm": run_test_mode_hhvm
    }[mode]

    return run_test_function(test_path)


def die(keep_going=False):
    global dead

    if not keep_going:
        dead = True


def run_test_mode(test, args):
    global dead

    assert not (args.mode == "php" and "no_php" in test.tags)
    if dead:
        return None

    mode = args.mode
    ok = True
    out_s = ""
    try:
        perfs = []
        if mode != "none":
            ok, s = test.run(args)
            out_s += s

            if not ok:
                die(args.keep_going)
                return ok

            if test.cur_perf:
                perfs.append(("now." + mode, test.cur_perf))

        for other_mode in args.compare_modes:
            perf = test.get_perf(other_mode)
            if perf:
                perfs.insert(0, (other_mode, perf))

        if perfs:
            if mode == "none":
                out_s += "Test [{}]\n".format(test.short_path)
            base_mode, base_perf = perfs[0]
            for mode, perf in perfs:
                time_status = "time {}({})".format(perf.time, perf_cmp(base_perf, perf, "time"))
                memory_status = "memory {}({})".format(perf.memory, perf_cmp(base_perf, perf, "memory"))

                out_s += "[{mode:>10s}]\t{time_status}\t{memory_status}\n".format(
                    mode=mode, time_status=time_status, memory_status=memory_status)

            out_s += "----------------------------------------\n"

    finally:
        print(out_s, end='')

    return ok


class Test:
    def __init__(self, path, tags=set()):
        self.path = Path(path).resolve()
        self.short_path = self.path.relative_to(DIRECTORY_WITH_TESTS)

        with self.path.open("rb") as f:
            header = f.readline().decode('utf-8')

        if header[0] == '@':
            self.tags = set(header[1:].split())
        else:
            self.tags = {"none"}
        self.tags |= tags

        self.ans_path = get_ans_path(self.path)
        if not self.ans_path.exists():
            self.tags |= {"no_ans"}

        self.cur_perf = None

        self.ok_label_path = self.path.with_suffix(".ok")

    def run(self, args):
        global dead

        mode = args.mode
        out_s = ""

        assert not (args.gen_answers and self.ans_path.exists())

        kphp_should_fail = mode == "kphp" and "kphp_should_fail" in self.tags

        if not args.gen_answers and not self.ans_path.exists() and not kphp_should_fail:
            out_s += "{}No answer{} for test [{path}]\n" \
                .format(BashColors.FAIL, BashColors.END_COLOR, path=self.short_path)

            die(args.keep_going)

            return False, out_s

        ok = True
        try:
            if dead:
                return None, out_s

            cur_ans_path, cur_perf_path = run_test_mode_helper(mode, self.path)

            if args.gen_answers:
                out_s += self.save_answer(cur_ans_path, mode)
            else:
                ok, s = self.check_answer(cur_ans_path, mode, args.keep_going)
                out_s += s

            if ok and args.rewrite_perf:
                out_s += self.save_perf(cur_perf_path, mode)

            self.cur_perf = get_perf_from_path(cur_perf_path)

        except CalledProcessError as e:
            print(e)

            if dead:
                return None, out_s

            if kphp_should_fail:
                out_s += "Test [{path}] [{mode}]: {0}Compilation failed, OK{1}\n".format(
                    BashColors.OK_GREEN, BashColors.END_COLOR, path=self.short_path, mode=mode)

                self.ok_label_path.touch()
            else:
                out_s += "\n{}Failed to run [{}] [{}]{}\n" \
                    .format(BashColors.FAIL, self.short_path, mode, BashColors.END_COLOR)

                path_to_info_about_fail = None
                if args.mode == "kphp":
                    path_to_info_about_fail = get_kphp_out_path()
                elif args.mode == "php":
                    path_to_info_about_fail = add_temp_prefix("err")

                if path_to_info_about_fail.exists():
                    if args.keep_going:
                        new_out_path = TMP_PATH / get_unique_name_of_test(self.path.with_suffix(".err"))
                        shutil.copyfile(str(path_to_info_about_fail), str(new_out_path))

                        out_s += "see file: {0}{out}{1}\n\n".format(BashColors.FAIL, BashColors.END_COLOR,
                                                                    out=new_out_path)
                    else:
                        with path_to_info_about_fail.open() as f:
                            out_s += "{0}OUTPUT:{1}{out}\n\n".format(BashColors.FAIL, BashColors.END_COLOR,
                                                                     out=f.read())

                die(args.keep_going)

                return False, out_s

        return ok, out_s

    def check_answer(self, ans_path, mode, keep_going=False):
        assert self.ans_path.exists()
        try:
            diff_cmd = ["diff", "--text", "-udBb", str(self.ans_path), str(ans_path)]
            subprocess.check_output(diff_cmd, stderr=subprocess.STDOUT)
            self.ok_label_path.touch()

            return True, "Test [{path}] [{mode}]: {0}OK{1}\n".format(BashColors.OK_GREEN, BashColors.END_COLOR,
                                                                     path=self.short_path, mode=mode)
        except CalledProcessError as e:
            die(keep_going)
            msg = "\nTest [{path}] [{mode}]: {0}WA{1}\n".format(BashColors.FAIL, BashColors.END_COLOR,
                                                                path=self.short_path, mode=mode)
            msg += e.output.decode("utf-8", 'ignore') + "\n"

            return False, msg

    def save_answer(self, ans_path, mode):
        shutil.copyfile(str(ans_path), str(self.ans_path))
        return "Answer for [{path}] is generated by [{mode}]\n".format(path=self.short_path, mode=mode)

    def save_perf(self, perf_path, mode):
        out_s = ""
        target_perf_path = get_perf_path(self.path, mode)
        perf = self.get_perf(mode)

        new_perf = get_perf_from_path(perf_path)
        if new_perf is None:
            out_s = "Perf file [{}] not exists\n".format(perf_path)

        if perf:
            if new_perf:
                time = min(perf.time, new_perf.time)
                memory = min(perf.memory, new_perf.memory)

                with target_perf_path.open("w") as f:
                    f.write("{} {}".format(time, memory))
        else:
            shutil.copyfile(str(perf_path), str(target_perf_path))

        return out_s

    def get_perf(self, mode):
        perf_path = get_perf_path(self.path, mode)
        return get_perf_from_path(perf_path)


def get_tests(tests, tags=None):
    if tags is None:
        return []

    tests = sorted(tests)
    tests = [Test(x, set(tags)) for x in tests]
    return tests


def remove_previous_ok_labels(tests):
    ok_tests = filter(lambda test: test.ok_label_path.exists(), tests)

    for test in ok_tests:
        test.ok_label_path.unlink()


def remove_ok_tests(tests_it, force=False):
    if force:
        tests_it, copy_tests_iterator = itertools.tee(tests_it)
        remove_previous_ok_labels(copy_tests_iterator)

        return tests_it
    else:
        return filter(lambda test: not test.ok_label_path.exists(), tests_it)


def check_excluded_tags_exist_in_tests(tests, exclude_tags):
    excluded_tags_in_tests = [exclude_tags & x.tags for x in tests]
    excluded_tags_in_tests = functools.reduce(lambda x, y: x | y, excluded_tags_in_tests, set())

    if excluded_tags_in_tests != exclude_tags:
        print("\n{0}WARNING: there are no tests with these tags:{1} {tags}\n"
              .format(BashColors.FAIL, BashColors.END_COLOR, tags=exclude_tags - excluded_tags_in_tests))

        if input("continue? [Y/n]") == "n":
            sys.exit(0)


def get_all_tests(args):
    tests = []
    if args.manual_tests:
        tests = get_tests(args.manual_tests, ["manual"])
        return list(tests)

    test_tags = [
        ["dl"],
        ["dl", "switch"],
        ["phc", "parsing"],
        ["phc", "codegen"],
        ["pk"],
        ["nn"],
        ["cl"],
    ]

    for tags in test_tags:
        path = DIRECTORY_WITH_TESTS / "/".join(tags)
        tests.extend(get_tests(path.glob("*.php"), tags))

    if args.mode == "php":
        args.exclude_tags.add("no_php")

    check_excluded_tags_exist_in_tests(tests, args.exclude_tags)

    tests = list(filter(lambda x: args.need_tags <= x.tags and not (args.exclude_tags & x.tags), tests))
    if not args.gen_answers:
        new_tests = list(remove_ok_tests(tests, args.force))

        if tests and not new_tests:
            print("all tests were passed in previous run. {0}Run with --force{1}".format(BashColors.OK_GREEN, BashColors.END_COLOR))

        tests = new_tests

    return list(tests)


def create_working_dirs():
    shutil.rmtree(str(TMP_PATH), ignore_errors=True)
    TMP_PATH.mkdir()

    if not ANSWERS_PATH.exists():
        ANSWERS_PATH.mkdir()

    if not PERF_PATH.exists():
        PERF_PATH.mkdir()

    for mode in SUPPORTED_MODES:
        tests_mask = PERF_PATH / mode
        if not tests_mask.exists():
            tests_mask.mkdir()


def run_test_task(test, args):
    res = run_test_mode(test, args)
    Result = namedtuple("Result", ['res', 'test'])
    return Result(res, test)


def run_tests(args, tests):
    global dead

    if args.gen_answers and args.force:
        shutil.rmtree(str(ANSWERS_PATH), ignore_errors=True)
    create_working_dirs()

    if args.gen_answers:
        existed_tests = filter(lambda t: t.ans_path.exists(), tests)
        for test in existed_tests:
            print("Answer for [{}] already exists".format(test.short_path))
        print()

        tests = list(filter(lambda t: not t.ans_path.exists(), tests))

    ok_cnt = 0
    wa_tests = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.cnt_workers) as executor:
        not_done = set(map(lambda test: executor.submit(run_test_task, test, args), tests))

        while not dead and not_done:
            done, not_done = concurrent.futures.wait(not_done, return_when=concurrent.futures.FIRST_COMPLETED)

            done = list(map(lambda f: f.result(), done))
            new_wa_tests = [f.test for f in done if f.res == False]
            wa_tests.extend(new_wa_tests)

            ok_cnt += sum([1 for f in done if f.res])

    print("Total tests were run: {}".format(ok_cnt))
    print("{0}OK: {ok_cnt} tests{1}".format(BashColors.OK_GREEN, BashColors.END_COLOR, ok_cnt=ok_cnt))
    print("{0}FAILED: {wa_cnt} tests {1}".format(BashColors.FAIL, BashColors.END_COLOR, wa_cnt=len(wa_tests)))

    if wa_tests:
        for test in wa_tests:
            print("\t{0}[{wa_path}]{1}".format(BashColors.FAIL, BashColors.END_COLOR, wa_path=test.short_path))


def print_tests(tests):
    for x in tests:
        print(x.path)
    print(len(tests))
    sys.exit(0)


def print_tags(tests):
    used_tags = functools.reduce(lambda x, y: x | y.tags, tests, set())
    print("existed tags in tests which will be run: ", used_tags)


def signal_handler(_s, _frame):
    die()


def parse_command_line_arguments():
    class CustomFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawDescriptionHelpFormatter):
        pass

    parser = argparse.ArgumentParser(description="""There are two modes: generate answers and check results.
    `./tester.py -A -a ok -m php` - generates answers, saves them to folder `answers`, 
                                    by running `php` on the .php files
    `./tester.py -a ok` - will check result of kphp program with answers generated by previous command
    """, formatter_class=CustomFormatter)

    parser.add_argument('--manual', dest='manual_tests', metavar='MANUAL_TEST', nargs='*',
                        help='manual tests which will be checked')

    parser.add_argument('-A', '--gen-answers', action='store_true',
                        help='generate answers if none')

    parser.add_argument('-P', '--rewrite-perf', action='store_true',
                        help='rewrite perf files')

    parser.add_argument('-a', '--need-tags', metavar='TAG',
                        default=set(), nargs='+',
                        help='test must have given <tags>')

    parser.add_argument('-d', '--exclude-tags', metavar='TAG',
                        default=set(), nargs='+',
                        help="test mustn't have given <tags>")

    parser.add_argument('-c', '--compare-modes', metavar='MODE', choices=SUPPORTED_MODES,
                        default=[], nargs='+',
                        help="compare with modes")

    parser.add_argument('-l', '--print-tests', action='store_true',
                        help="just print test names")

    parser.add_argument('-k', '--keep-going', action='store_true',
                        help="keep going")

    parser.add_argument('-f', '--force', action='store_true',
                        help="""with --gen-answers force to generate new answers
                                without --gen-answers force to run all tests despite it can be passed before""")

    parser.add_argument('-m', '--mode', default='kphp', choices=SUPPORTED_MODES,
                        help="run in mode")

    parser.add_argument('--disable-coloring', action='store_true', default=None,
                        help="disable coloring (useful when stdout not in tty)")

    parser.add_argument('-w', '--cnt-workers', default=16, type=int,
                        help="count of workers for executing tests")

    parser.add_argument('-e', '--engine-path', default="../../",
                        help="path to engine directory")

    res = parser.parse_args(sys.argv[1:])
    res.need_tags = set(res.need_tags)
    res.exclude_tags = set(res.exclude_tags)

    if res.manual_tests:
        res.need_tags.add("manual")

    return res


def main():
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    parsed_args = parse_command_line_arguments()

    if parsed_args.disable_coloring:
        BashColors.disable()

    init_paths(parsed_args.engine_path)

    tests = get_all_tests(parsed_args)

    if not tests:
        print("No tests to run")
        print("need tags: ", parsed_args.need_tags)
        print("exclude tags: ", parsed_args.exclude_tags)
        return

    if parsed_args.print_tests:
        print_tests(tests)

    print_tags(tests)

    if parsed_args.mode == "kphp":
        run_make_kphp()
    print("start testing")

    run_tests(parsed_args, tests)


def check_minimum_required_python_version():
    min_python_version = (3, 4)
    if sys.version_info < min_python_version:
        sys.exit("Python {}.{} or later is required.\n".format(*min_python_version))


if __name__ == "__main__":
    check_minimum_required_python_version()
    main()
