import typing

from python.lib.testcase import WebServerAutoTestCase


def _operation(kind: str, kwargs: typing.Dict[str, typing.Any], expected_result: typing.Any):
    return {
        "kind": kind,
        **kwargs,
        "expected": expected_result,
    }


def _rpc_parse(new_rpc_data: str, expected_result: bool):
    return _operation('rpc_parse', dict(new_rpc_data=new_rpc_data), expected_result)


def _fetch_int(expected_result: int):
    return _operation('fetch_int', dict(), expected_result)


def _fetch_long(expected_result: int):
    return _operation('fetch_long', dict(), expected_result)


def _fetch_double(expected_result: float):
    return _operation('fetch_double', dict(), expected_result)


def _fetch_string(expected_result: str):
    return _operation('fetch_string', dict(), expected_result)


class TestTlPrimitives(WebServerAutoTestCase):
    def call(self, *operations):
        response = self.web_server.http_post(json=operations)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.text, "ok")

    def test_rpc_parse(self):
        self.call(
            _rpc_parse('\x12\x34\x56\x78\x42\x00\x00\x00\x00\x00\x00\x00\x68\x56\x5b\x56\x06\x22\x09\x40\x03abc', True),
            _fetch_int(0x78563412),
            _fetch_long(0x42),
            _fetch_double(3.14161365),
            _fetch_string('abc'),
        )
