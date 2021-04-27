from python.lib.testcase import KphpServerAutoTestCase


class TestJobResumable(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--job-workers-num": 2
        })

    def test_jobs_in_wait_queue(self):
        stats_before = self.kphp_server.get_stats()
        resp = self.kphp_server.http_post(
            uri="/test_jobs_in_wait_queue",
            json={
                "tag": "x2_with_sleep",
                "job-sleep-time-sec": 1,
                "data": [[1, 2, 3, 4], [7, 9, 12]]})

        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [1 * 1, 2 * 2, 3 * 3, 4 * 4], "stats": []},
                {"data": [7 * 7, 9 * 9, 12 * 12], "stats": []},
            ]})
        self.kphp_server.assert_stats(
            initial_stats=stats_before,
            expected_added_stats={
                "kphp_server.job_workers_memory_messages_buffers_acquired": 4,
                "kphp_server.job_workers_memory_messages_buffers_released": 4,
                "kphp_server.job_workers_memory_messages_buffer_acquire_fails": 0
            })

    def test_several_waits_for_one_job(self):
        stats_before = self.kphp_server.get_stats()
        resp = self.kphp_server.http_post(
            uri="/test_several_waits_for_one_job",
            json={
                "tag": "x2_with_sleep",
                "job-sleep-time-sec": 5,
                "data": [[1, 2, 3, 4]]})

        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [1 * 1, 2 * 2, 3 * 3, 4 * 4], "stats": []},
            ]})
        self.kphp_server.assert_stats(
            initial_stats=stats_before,
            expected_added_stats={
                "kphp_server.job_workers_memory_messages_buffers_acquired": 2,
                "kphp_server.job_workers_memory_messages_buffers_released": 2,
                "kphp_server.job_workers_memory_messages_buffer_acquire_fails": 0
            })
