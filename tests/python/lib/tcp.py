import contextlib
import socketserver
import threading
import time
import typing


class HelloHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # For test_non_blocking_connection
        time.sleep(0.05)
        self.request.send(b'Hello!')


@contextlib.contextmanager
def serving(port: typing.Optional[int]) -> typing.Generator[typing.Optional[socketserver.TCPServer], None, None]:
    if not port:
        yield
        return
    server = socketserver.TCPServer(('127.0.0.1', port), HelloHandler)
    try:
        server_thread = threading.Thread(target=server.serve_forever)
        server_thread.start()
        yield server
    finally:
        server.shutdown()
        server.server_close()
