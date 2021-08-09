from multiprocessing.dummy import Pool as ThreadPool

from python.lib.testcase import KphpServerAutoTestCase


class TestMessagesReferences(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 18,
            "--job-workers-ratio": 0.16,
            "--verbosity-job-workers=2": True,
        })

    def do_test(self, it):
        resp = self.kphp_server.http_get("/test_reference_invariant")
        self.assertEqual(resp.status_code, 200)

    def test_reference_invariant(self):
        requests_count = 1000
        with ThreadPool(5) as pool:
            for _ in pool.imap_unordered(self.do_test, range(requests_count)):
                pass

        self.kphp_server.assert_stats(
            prefix="kphp_server.workers_job_",
            expected_added_stats={
                "memory_messages_shared_messages_buffers_acquired": requests_count * 2,
                "memory_messages_shared_messages_buffers_released": requests_count * 2,
                "jobs_queue_size": 0,
                "jobs_sent": requests_count,
                "jobs_replied": requests_count,
                "pipe_errors_server_write": 0,
                "pipe_errors_server_read": 0,
                "pipe_errors_client_write": 0,
                "pipe_errors_client_read": 0,
            })
        self.assertKphpNoTerminatedRequests()
