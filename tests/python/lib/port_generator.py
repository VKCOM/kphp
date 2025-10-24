import socket
import pathlib

import portalocker


def get_port():
    port_tracker_path = "/tmp/__kphp_functional_tests_ports_tracker"

    start_port = 25000
    end_port = 40000

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    pathlib.Path(port_tracker_path).touch(exist_ok=True)

    fh = open(port_tracker_path, "r+")
    portalocker.lock(fh, portalocker.LOCK_EX)
    port_used_last = fh.readline()
    port = start_port if not port_used_last else int(port_used_last)

    attempts = 0
    while attempts < 100:
        port += 1

        if port > end_port:
            port = start_port

        if sock.connect_ex(('127.0.0.1', port)) != 0:
            fh.seek(0)
            fh.write(str(port))
            fh.truncate()
            fh.close()
            return int(port)
        attempts += 1

    fh.close()

    raise RuntimeError("all {} tries to pick a port failed".format(attempts))
