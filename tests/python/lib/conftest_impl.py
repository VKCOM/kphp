import os
import pathlib
import pytest

from .file_utils import search_k2_bin
from . import k2_builtin


@pytest.fixture(autouse=True)
def skip_k2_unsupported_test(request):
    if search_k2_bin() is not None:
        k2_skip_mark = request.node.get_closest_marker("k2_skip")
        if k2_skip_mark:
            pytest.skip("K2 skipped test")


@pytest.fixture(scope="class", autouse=True)
def skip_k2_unsupported_test_suite(request):
    if search_k2_bin() is not None:
        k2_skip_mark = request.node.get_closest_marker("k2_skip_suite")
        if k2_skip_mark:
            request.cls.custom_setup = lambda: None
            request.cls.custom_teardown = lambda: None
            pytest.skip("K2 skipped test")

@pytest.fixture(autouse=True)
def skip_kphp_unsupported_test(request):
    if search_k2_bin() is None:
        kphp_skip_mark = request.node.get_closest_marker("kphp_skip")
        if kphp_skip_mark:
            pytest.skip("KPHP skipped test")


@pytest.fixture(scope="class", autouse=True)
def skip_kphp_unsupported_test_suite(request):
    if search_k2_bin() is None:
        kphp_skip_mark = request.node.get_closest_marker("kphp_skip_suite")
        if kphp_skip_mark:
            request.cls.custom_setup = lambda: None
            request.cls.custom_teardown = lambda: None
            pytest.skip("KPHP skipped test")


@pytest.fixture(scope='session')
def k2_builtin_calls(request: pytest.FixtureRequest):
    builtin_calls = k2_builtin.Calls()

    yield builtin_calls

    target_dir = request.config.rootpath.parent / "_tmp"

    target_dir.mkdir(parents=True, exist_ok=True)

    filename = "k2_builtin_calls.json"
    output_path = target_dir / filename
    
    with open(output_path, "w", encoding="utf-8") as f:
        builtin_calls.dump(f)
