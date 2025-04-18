import os
import time
import pytest

from python.lib.testcase import WebServerAutoTestCase

@pytest.mark.k2_skip_suite
class TestWarmUpOnGracefulRestart(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": 15,
            "-v": True,
        })

    def prepare_for_test(self, *, workers_part, instance_cache_part, timeout_sec):
        self.web_server.update_options({
            "--warmup-workers-ratio": workers_part,
            "--warmup-instance-cache-elements-ratio": instance_cache_part,
            "--warmup-timeout": timeout_sec,
        })
        self.web_server.restart()
        return self.web_server.pid

    def trigger_kphp_to_store_element_in_instance_cache(self):
        resp = self.web_server.http_get(uri='/store-in-instance-cache')
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "1")

    def test_warmup_timeout_expired(self):
        timeout_sec = 1
        old_pid = self.prepare_for_test(workers_part=0.01, instance_cache_part=1, timeout_sec=timeout_sec)

        self.trigger_kphp_to_store_element_in_instance_cache()
        time.sleep(1)  # to be sure master saved stat in shared memory
        self.web_server.start()
        time.sleep(timeout_sec * 4)

        # Can exit only on timeout because instance cache is always cold (new master stores to instance cache nothing)
        self.assertEqual(os.waitpid(old_pid, os.WNOHANG)[0], old_pid)
        self.web_server.assert_log(["[warmup_timeout_expired = 1]"], "Warm up timeout was not expired on restart")

    def test_warmup_instance_cache_becomes_hot(self):
        old_pid = self.prepare_for_test(workers_part=0.01, instance_cache_part=0.001, timeout_sec=30)

        self.trigger_kphp_to_store_element_in_instance_cache()
        time.sleep(1)  # to be sure master saved stat in shared memory
        self.web_server.start()

        time.sleep(5)
        # check it's still waiting while new master warms up
        self.assertEqual(os.waitpid(old_pid, os.WNOHANG)[0], 0)

        # to be sure new workers will get at least one request
        for _ in range(100):
            self.trigger_kphp_to_store_element_in_instance_cache()

        time.sleep(4)
        # here it must be hot enough
        self.assertEqual(os.waitpid(old_pid, os.WNOHANG)[0], old_pid)
        self.web_server.assert_log(["[is_instance_cache_hot_enough = 1]"], "Instance cache was not warmed up")
