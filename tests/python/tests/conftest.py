import os
import pytest

@pytest.fixture(autouse=True)
def skip_k2_unsupported_test(request):
    if os.getenv("K2_BIN") is not None:
        k2_skip_mark = request.node.get_closest_marker("k2_skip")
        if k2_skip_mark:
            pytest.skip("K2 skipped test")

@pytest.fixture(scope="class", autouse=True)
def skip_k2_unsupported_test_suite(request):
    if os.getenv("K2_BIN") is not None:
        k2_skip_mark = request.node.get_closest_marker("k2_skip_suite")
        if k2_skip_mark:
            request.cls.custom_setup = lambda: None
            request.cls.custom_teardown = lambda: None
            pytest.skip("K2 skipped test")
