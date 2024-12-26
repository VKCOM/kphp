import requests.exceptions
from python.lib.testcase import KphpServerAutoTestCase


class TestOomHandler(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 2,
            "--job-workers-ratio": 0.5,
            "--hard-memory-limit": "10m",
            "--oom-handling-memory-ratio": 0.1,
            "--time-limit": 1,
        })

    def _generic_test(self, params: str):
        resp = self.kphp_server.http_get("/test_oom_handler?" + params)
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_log([
            "allocations_cnt=[1-9]\\d?,memory_allocated=8\\d{5},estimate_memory_usage\\(arr\\)=8\\d{5}",
            "Critical error during script execution: memory limit exit",
            "Warning: Can't allocate \\d+ bytes",
        ], timeout=5)

    def test_basic(self):
        self._generic_test("test_case=basic")

    def test_with_rpc_request(self):
        self._generic_test("test_case=with_rpc_request&master_port={}".format(self.kphp_server.master_port))
        self.kphp_server.assert_log(["rpc_request_succeeded=1"], timeout=5)

    def test_oom_inside_oom_handler(self):
        try:
            self.kphp_server.http_get("/test_oom_handler?test_case=oom_inside_oom_handler")
        except requests.exceptions.ConnectionError:
            pass
        self.kphp_server.assert_log([
            'Warning: Critical error "Out of memory error happened inside OOM handler"'
        ], timeout=5)

    def test_script_memory_dealloc(self):
        self._generic_test("test_case=script_memory_dealloc")
        self.kphp_server.assert_log(["refcnt_before_unset=1"], timeout=5)

    def test_script_memory_realloc(self):
        self._generic_test("test_case=script_memory_realloc")
        self.kphp_server.assert_log(["realloc_allocations_cnt=1,realloc_mem_allocated=[1-9]\\d*"], timeout=5)

    def test_with_job_request(self):
        self._generic_test("test_case=with_job_request")
        self.kphp_server.assert_log(["job_request_succeeded=1"], timeout=5)

    def test_with_instance_cache(self):
        self._generic_test("test_case=with_instance_cache")
        self.kphp_server.assert_log(["instance_cache_store_succeeded=1"], timeout=5)

    def test_resume_yielded_script_fork_from_oom_handler(self):
        self._generic_test("test_case=resume_yielded_script_fork_from_oom_handler&master_port={}".format(self.kphp_server.master_port))
        self.kphp_server.assert_log(["fork_started_succesfully=1",
                                     "start OOM handler from fork_id=0"], timeout=5)

    def test_resume_suspended_script_fork_from_oom_handler(self):
        self._generic_test("test_case=resume_suspended_script_fork_from_oom_handler&master_port={}".format(self.kphp_server.master_port))
        self.kphp_server.assert_log(["fork_started_succesfully=1",
                                     "start OOM handler from fork_id=0"], timeout=5)

    def test_oom_from_fork(self):
        self._generic_test("test_case=oom_from_fork".format(self.kphp_server.master_port))
        self.kphp_server.assert_log(["fork_started_succesfully=1",
                                     "start OOM handler from fork_id=0"], timeout=5)

    def test_timeout_in_oom_handler(self):
        resp = self.kphp_server.http_get("/test_oom_handler?test_case=timeout_in_oom_handler")
        self.assertEqual(resp.status_code, 500)
        self.kphp_server.assert_log([
            "allocations_cnt=[1-9]\\d?,memory_allocated=8\\d{5},estimate_memory_usage\\(arr\\)=8\\d{5}",
            "timeout exit",
        ], timeout=5)
