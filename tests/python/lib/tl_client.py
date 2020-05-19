import json
import os
import subprocess

from .file_utils import search_tl_client
from .colors import blue, green


def _tl_serialize_element(element):
    if isinstance(element, bool):
        return element and "( boolTrue )" or "( boolFalse )"
    elif isinstance(element, tuple):
        return _tl_serialize_struct(element)
    else:
        return str(element)


def _tl_serialize_struct(request):
    encoded = ["(", request[0]]
    if request[0] == "vector":
        encoded += [str(len(request[1])), "["]
        encoded += [_tl_serialize_element(v) for v in request[1]]
        encoded.append("]")
    else:
        for k, v in request[1]:
            encoded += [k, ":", _tl_serialize_element(v)]
    encoded.append(")")
    return " ".join(encoded)


def send_rpc_request(request, port):
    tl_client_bin = search_tl_client()
    encoded_request = _tl_serialize_struct(request)

    print("\nSending rpc request to port {}: {}".format(port, blue(encoded_request)))
    cmd = [tl_client_bin, "--stdin", "--json-encoded", "--port", str(port)]
    if not os.getuid():
        cmd += ["--user", "root", "--group", "root"]
    p = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    stdout_data, stderr_data = p.communicate(input=encoded_request.encode())
    if stderr_data:
        raise RuntimeError("Can't send rpc request: " + stderr_data.decode())
    if p.returncode:
        raise RuntimeError("Can't send rpc request: got nonzero exit code")
    if not stdout_data:
        raise RuntimeError("Got empty stdout")
    stdout_data = stdout_data.strip().decode()
    if stdout_data == "Can't serialize":
        raise RuntimeError("Can't serialize request")
    print("\nGot rpc response: {}".format(green(stdout_data)))
    return json.loads(stdout_data)
