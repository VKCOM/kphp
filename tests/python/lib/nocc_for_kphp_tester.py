import subprocess
import os
import random


def nocc_env(nocc_env_name, default):
    return os.getenv(nocc_env_name, default)


def nocc_prepend_to_cxx(cxx_name):
    return "{} {}".format(nocc_env('NOCC_EXECUTABLE', "nocc"), cxx_name)


def nocc_make_env():
    nocc_servers_filename = nocc_env('NOCC_SERVERS_FILENAME', None)
    if nocc_servers_filename == "" or nocc_servers_filename is None:
        raise RuntimeError("env NOCC_SERVERS_FILENAME not set")

    return {
        "NOCC_GO_EXECUTABLE": nocc_env('NOCC_GO_EXECUTABLE', "/usr/bin/nocc-daemon"),
        "NOCC_SERVERS_FILENAME": nocc_servers_filename,
        "NOCC_CLIENT_ID": nocc_env("NOCC_CLIENT_ID", "kphp-tester-" + ''.join(random.choices('0123456789', k=6))),
        "NOCC_LOG_FILENAME": nocc_env('NOCC_LOG_FILENAME', "/tmp/nocc-kphp-tests.log"),
        "NOCC_LOG_VERBOSITY": "1",
        "NOCC_DISABLE_OBJ_CACHE": "1",
    }


# The first invocation of `nocc` automatically launches a daemon,
# so that the end user doesn't have to do it manually.
# However, whyever, python waits for that auto-launched daemon to quit:
# probably, wait() waits not only for direct children, but for sub-subprocesses also.
# So, this chain happens:
# python launches one phpt -> kphp starts make -> `nocc` is launched -> `nocc-daemon` is launched once.
# After that, this (the first) test becomes "hanging", python doesn't quit until a daemon dies (in 15 seconds after all tests are finished).
# (when a daemon dies, that first test immediately becomes "accepted").
#
# To overcome this, we launch a daemon manually in advance, so that the first `nocc` invocation
# already would connect to it, and no tests will have background subprocesses.
# If a daemon quits in the middle of pipeline, it will be launched again by the next `nocc`,
# and everything would still work, except that there will be a 15 sec hang after all tests finish.
def nocc_start_daemon_in_background():
    env = os.environ.copy()
    env.update(nocc_make_env())

    proc = subprocess.Popen(
        [nocc_env('NOCC_EXECUTABLE', "nocc"), "start"],
        env=env, stdout=subprocess.DEVNULL)
    return proc
