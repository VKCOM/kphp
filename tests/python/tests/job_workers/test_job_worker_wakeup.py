import pytest
from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestJobWorkerWakeup(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 8,
            "--job-workers-ratio": 0.25,
            "--verbosity-job-workers=2": True,
        })

    def test_job_worker_wakeup_on_merged_events(self):
        resp = self.web_server.http_post(
            uri="/test_job_worker_wakeup_on_merged_events",
            json={
                "data": [[1, 2, 3, 4], [7, 9, 12], [13, 14, 15]],
            })
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [1 * 1, 2 * 2, 3 * 3, 4 * 4], "stats": []},
                {"data": [7 * 7, 9 * 9, 12 * 12], "stats": []},
                {"data": [13 * 13, 14 * 14, 15 * 15], "stats": []},
            ]})
