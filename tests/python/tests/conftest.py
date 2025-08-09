import os
import pytest

from python.lib.conftest_impl import skip_k2_unsupported_test, skip_k2_unsupported_test_suite, skip_kphp_unsupported_test, skip_kphp_unsupported_test_suite


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_protocol(item: pytest.Item):
    tw = item.config.get_terminal_writer()

    tw.write(f"START TEST: {item.nodeid}")
    yield
    tw.write("\n")
    tw.write(f"FINISH TEST: {item.nodeid}")
    tw.write("\n")
