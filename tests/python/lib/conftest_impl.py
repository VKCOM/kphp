import os
import shutil
import pathlib
import pytest

from .file_utils import search_k2_bin
from . import std_function
from . import testcase


def _sync_data(tmp_dir: str, test_parent_dir: pathlib.Path):
    data_dir = test_parent_dir / "php/data"
    tmp_data_dir = pathlib.Path(tmp_dir) / "data"

    if data_dir.is_dir():
        tmp_data_dir.mkdir(parents=True, exist_ok=True)
        for full_data_file in data_dir.iterdir():
            full_tmp_file = tmp_data_dir / full_data_file.name
            if full_tmp_file.exists():
                continue

            if full_data_file.is_file():
                shutil.copy(full_data_file, tmp_data_dir)
            elif full_data_file.is_dir():
                shutil.copytree(full_data_file, full_tmp_file)


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


@pytest.fixture(scope="session")
def session_tmp_dir(request: pytest.FixtureRequest):
    return request.config.rootpath.parent / "_tmp"


@pytest.fixture(scope="class")
def class_tmp_dir(request: pytest.FixtureRequest, session_tmp_dir: pathlib.Path):
    relative_subpath = pathlib.Path(request.module.__file__).parent.relative_to(request.config.rootpath)

    return session_tmp_dir / relative_subpath


@pytest.fixture(scope="class")
def kphp_build_working_dir(class_tmp_dir: pathlib.Path):
    res = class_tmp_dir / "working_dir"
    res.mkdir(parents=True, exist_ok=True)
    return res


@pytest.fixture(scope="class")
def artifacts_dir(class_tmp_dir: pathlib.Path):
    res = class_tmp_dir / "artifacts"
    res.mkdir(parents=True, exist_ok=True)
    return res


@pytest.fixture(scope="class")
def tmp_dir_root(request: pytest.FixtureRequest, artifacts_dir: pathlib.Path):
    test_suite_name = pathlib.Path(request.module.__file__).stem
    res = artifacts_dir / "tmp_{}".format(test_suite_name)
    res.mkdir(parents=True, exist_ok=True)
    return res



@pytest.fixture(scope="class")
def kphp_server_working_dir(request: pytest.FixtureRequest, tmp_dir_root: pathlib.Path):
    server_working_dir = testcase.make_test_tmp_dir(tmp_dir_root)
    _sync_data(server_working_dir, pathlib.Path(request.module.__file__).parent)
    return server_working_dir


@pytest.fixture(scope="session")
def std_function_invocations(session_tmp_dir: pathlib.Path):
    if "KPHP_TRACKED_BUILTINS_LIST" not in os.environ:
        yield None
        return

    function_invocations = std_function.Invocations(os.environ["KPHP_TRACKED_BUILTINS_LIST"])

    yield function_invocations

    output_dir = session_tmp_dir / "artifacts"
    output_dir.mkdir(parents=True, exist_ok=True)

    filename = "std_function_invocations.json"
    output_path = output_dir / filename
    
    with open(output_path, "w", encoding="utf-8") as f:
        function_invocations.dump(f)
