import signal
import pytest
from time import sleep
from threading import Thread

from python.lib.testcase import WebServerAutoTestCase

@pytest.mark.k2_skip_suite
class TestRequest(WebServerAutoTestCase):
    def send_http_request(self, sleep_time):
        self.resp = self.web_server.http_get("/sleep?time={}".format(sleep_time))

    def trigger_graceful_shutdown_with_delay(self):
        sleep(0.2)  # to be more sure that sigterm will be received in the middle of request processing
        self.web_server.send_signal(signal.SIGTERM)

    def test_graceful_shutdown_during_request_processing(self):
        sleep_time = 15
        actions = [
            Thread(target=self.send_http_request, args=(sleep_time,)),
            Thread(target=self.trigger_graceful_shutdown_with_delay)
        ]
        actions[0].start()
        actions[1].start()
        actions[1].join()
        actions[0].join()
        self.web_server.wait_termination(20)
        self.web_server.start()
        self.assertEqual(self.resp.status_code, 200)
        self.assertEqual(self.resp.text, "before sleep {}\n"
                                         "after sleep".format(sleep_time))
