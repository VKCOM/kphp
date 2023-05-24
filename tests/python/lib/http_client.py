from requests_toolbelt.utils import dump
import requests
import socket
from urllib.request import urlopen, Request

from .colors import blue

class RawResponse:
    def __init__(self, raw_bytes):
        self.raw_bytes = raw_bytes
        head, _, body = raw_bytes.partition(b"\r\n\r\n")

        first_line, _, headers_data = head.partition(b"\r\n")
        self.method, _, status = first_line.partition(b' ')
        status_code, _, status_line = status.partition(b' ')
        self.status_code = int(status_code)
        self.reason = status_line
        self.headers = {}
        self.content = body

        for i in headers_data.splitlines():
            k, v = i.split(b': ')
            self.headers[k.decode()] = v.decode()


def send_http_request(port, uri='/', method='GET', timeout=30, **kwargs):
    # session = requests.session()
    # url = 'http://127.0.0.1:{}{}'.format(port, uri)
    # print("\nSending HTTP request: [{}]".format(blue(url)))
    # r = session.request(method=method, url=url, timeout=timeout, **kwargs)
    # session.close()
    url = 'http://127.0.0.1:{}{}'.format(port, uri)
    r = urlopen(Request(method=method, url=url)).read().decode('utf-8')
    print("HTTP request debug:")
    print("=============================")
    print(*[i for i in dump.dump_all(r).splitlines(True)], sep="\n")
    print("=============================")
    return r


def send_http_request_raw(port, request):
    crlf = b"\r\n"
    msg = (crlf.join(request) + crlf * 2)

    print("\nSending Raw HTTP request")
    print("=============================")
    print(*msg.splitlines(True), sep="\n")
    print("=============================")

    s = socket.socket()
    s.connect(('127.0.0.1', port))
    s.send(msg)
    response_bytes = b''
    buffer_size = 4096
    buffer = s.recv(buffer_size)
    response_bytes += buffer

    while buffer and len(buffer) == buffer_size:
        buffer = s.recv(buffer_size)
        response_bytes += buffer

    s.close()
    print("\nGot Raw HTTP response")
    print("=============================")
    print(*response_bytes.splitlines(True), sep="\n")
    print("=============================")

    return RawResponse(response_bytes)
