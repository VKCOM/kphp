from python.lib.testcase import KphpServerAutoTestCase


class TestJobResumable(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 4,
            "--job-workers-ratio": 0.5,
            "--verbosity-job-workers=2": True,
            "--verbosity-php-runner=3": True,
        })

    def _do_test(self, *, uri, job_sleep_time, data):
        stats_before = self.kphp_server.get_stats()
        resp = self.kphp_server.http_post(
            uri=uri,
            json={
                "tag": "x2_with_sleep",
                "job-sleep-time-sec": job_sleep_time,
                "data": data})

        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [x ** 2 for x in cur_data], "stats": []}
                for cur_data in data
            ]})
        self.kphp_server.assert_stats(
            initial_stats=stats_before,
            expected_added_stats={
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_acquired": len(data) * 2,
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_released": len(data) * 2,
                "kphp_server.workers_job_memory_messages_shared_messages_buffer_acquire_fails": 0
            })

    def test_jobs_in_wait_queue(self):
        self._do_test(uri="/test_jobs_in_wait_queue", job_sleep_time=1, data=[[1, 2, 3, 4], [7, 9, 12]])

    def test_nonblocking_wait_ready_job_after_cpu_work(self):
        self._do_test(uri="/test_nonblocking_wait_ready_job_after_cpu_work", job_sleep_time=0.5, data=[[1, 2, 3, 4]])

    def test_nonblocking_wait_ready_job_after_net_work(self):
        self._do_test(uri="/test_nonblocking_wait_ready_job_after_net_work", job_sleep_time=0.5, data=[[1, 2, 3, 4]])

    def test_several_waits_on_one_job(self):
        self._do_test(uri="/test_several_waits_on_one_job", job_sleep_time=0.5, data=[[1, 2, 3, 4]])
