import os
import subprocess
import re
import sys
import shutil

_SUPPORTED_PHP_VERSIONS = ["php7.4", "php8", "php8.1", "php8.2", "php8.3"]


def _check_file(file_name, file_dir, file_checker):
    file_path = os.path.join(file_dir, file_name)
    if file_checker(file_path):
        return file_path

    raise RuntimeError("Can't find " + file_path)


def _check_bin(bin_path):
    if os.path.isfile(bin_path):
        ret_code = subprocess.call(
            ["bash", "-c", "{} -h".format(bin_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        return ret_code == 0 or ret_code == 2
    return False


_ENGINE_INSTALL_PATH = "/usr/share/engine" if os.environ.get("KPHP_TESTS_INTERGRATION_TESTS_ENABLED") else None
_DEFAULT_KPHP_REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "../../../"))
_KPHP_REPO = os.environ.get("KPHP_TESTS_KPHP_REPO", _DEFAULT_KPHP_REPO)


def search_kphp2cpp():
    return _check_file("objs/bin/kphp2cpp", _KPHP_REPO, _check_bin)


def search_engine_bin(engine_name):
    return _check_file("bin/" + engine_name, _ENGINE_INSTALL_PATH, _check_bin) if _ENGINE_INSTALL_PATH else engine_name


def search_tl_client():
    return _check_file("bin/tlclient", _ENGINE_INSTALL_PATH, _check_bin) if _ENGINE_INSTALL_PATH else "tlclient"


def search_combined_tlo(working_dir):
    if _ENGINE_INSTALL_PATH:
        return _check_file("combined.tlo", _ENGINE_INSTALL_PATH, os.path.isfile)
    tl_compiler_path = _check_file("objs/bin/tl-compiler", _KPHP_REPO, _check_bin)
    common_tl = _check_file("common/tl-files/common.tl", _KPHP_REPO, os.path.isfile)
    subprocess.call(
        ["bash", "-c", "{} -e {} {}".format(tl_compiler_path, working_dir + "/combined.tlo", common_tl)],
        env={"ASAN_OPTIONS": "detect_leaks=0"}
    )
    return _check_file("combined.tlo", working_dir, os.path.isfile)


def replace_in_file(file, old, new):
    with open(file, 'r+') as conf:
        content = conf.read()
        conf.seek(0)
        conf.write(content.replace(old, new))
        conf.truncate()


def gen_random_aes_key_file(directory):
    key_file = os.path.join(directory, "aes-key")
    with open(key_file, 'wb') as f:
        f.write(os.urandom(200))
    return key_file


def error_can_be_ignored(ignore_patterns, binary_error_text):
    if not binary_error_text:
        return True

    for line in binary_error_text.split(b'\n'):
        if not line:
            continue
        is_line_ok = False
        for pattern in ignore_patterns:
            if re.fullmatch(pattern, line.decode()):
                is_line_ok = True
                break

        if not is_line_ok:
            return False
    return True


def can_ignore_sanitizer_log(sanitizer_log_file):
    with open(sanitizer_log_file, 'rb') as f:
        ignore_sanitizer = error_can_be_ignored(
            ignore_patterns=[
                "^==\\d+==WARNING: ASan doesn't fully support makecontext/swapcontext functions and may produce false positives in some cases\\!$",
                "^==\\d+==WARNING: ASan is ignoring requested __asan_handle_no_return: stack top.+$",
                "^False positive error reports may follow$",
                "^For details see .+$"
            ],
            binary_error_text=f.read())

    if ignore_sanitizer:
        os.remove(sanitizer_log_file)

    return ignore_sanitizer


def search_php_bin(php_version: str):
    if sys.platform == "darwin":
        return shutil.which("php")

    # checking from oldest to newest versions
    for spv in sorted(_SUPPORTED_PHP_VERSIONS):
        if spv < php_version:
            continue

        exe_path = shutil.which(spv)
        if exe_path is not None:
            return exe_path

    return None
