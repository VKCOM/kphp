import os
import subprocess
import re


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


_ENGINE_REPO = os.environ.get("KPHP_TESTS_ENGINE_REPO")
_DEFAULT_KPHP_REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "../../../"))
_KPHP_REPO = os.environ.get("KPHP_TESTS_KPHP_REPO", _DEFAULT_KPHP_REPO)


def search_kphp2cpp():
    return _check_file("objs/bin/kphp2cpp", _KPHP_REPO, _check_bin)


def search_engine_bin(engine_name):
    return _check_file("objs/bin/" + engine_name, _ENGINE_REPO, _check_bin)


def search_tl_client():
    return _check_file("objs/bin/tlclient", _ENGINE_REPO, _check_bin)


def search_combined_tlo():
    return _check_file("objs/bin/combined.tlo", _ENGINE_REPO, os.path.isfile)


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


def read_distcc_hosts(distcc_hosts_file):
    distcc_hosts = []
    if distcc_hosts_file:
        if not os.path.exists(distcc_hosts_file):
            raise RuntimeError("Can't find distcc host list file '{}'".format(distcc_hosts_file))

        with open(distcc_hosts_file, "r") as f:
            distcc_hosts = f.readlines()
    return distcc_hosts


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


def make_distcc_env(distcc_hosts, distcc_dir):
    os.makedirs(distcc_dir, exist_ok=True)
    return {
        "DISTCC_HOSTS": "--randomize localhost/1 {}".format(" ".join(distcc_hosts)),
        "DISTCC_DIR": distcc_dir,
        "DISTCC_LOG": os.path.join(distcc_dir, "distcc.log")
    }
