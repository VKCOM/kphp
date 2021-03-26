from python.lib.testcase import KphpServerAutoTestCase


class TestJobResumable(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--job-workers-num": 2
        })

    def test_jobs_in_wait_queue(self):
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
        self.kphp_server.assert_stats({"kphp_server.job_workers_currently_memory_slices_used": self.cmpEq(0)})

    def test_several_waits_for_one_job(self):
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
        self.kphp_server.assert_stats({"kphp_server.job_workers_currently_memory_slices_used": self.cmpEq(0)})

