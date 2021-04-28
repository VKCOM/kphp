import signal
from python.lib.testcase import KphpServerAutoTestCase


class TestJobGracefulTermination(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 4,
            "--job-workers-ratio": 0.5,
            "--verbosity-job-workers=2": True,
        })

    def _test_job_graceful_termination_impl(self, termination_type):
        resp = self.kphp_server.http_post(
            uri="/test_job_graceful_shutdown",
            json={
                "data": [[1, 2, 3, 4], [7, 9, 12]],
                "tag": "x2_with_work_after_response",
                "job-sleep-time-sec": 5
            })
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {
            "jobs-result": [
                {"data": [1 * 1, 2 * 2, 3 * 3, 4 * 4], "stats": []},
                {"data": [7 * 7, 9 * 9, 12 * 12], "stats": []},
            ]})
        if termination_type == 'shutdown':
            self.kphp_server.send_signal(signal.SIGTERM)
            self.kphp_server.wait_termination(10)
            self.kphp_server.start()
        elif termination_type == 'restart':
            self.kphp_server.start()
        else:
            assert False
        self.kphp_server.assert_log(["start work after response", "finish work after response"], "Work after response wasn't completed")

    def test_job_graceful_shutdown(self):
        self._test_job_graceful_termination_impl(termination_type='shutdown')

    def test_job_graceful_restart(self):
        self._test_job_graceful_termination_impl(termination_type='restart')
