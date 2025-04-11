from python.lib.testcase import KphpServerAutoTestCase
from concurrent.futures import ThreadPoolExecutor
import time


class TestShutdownFunctionsCaptureSharedMem(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 4,
        })

    # A specific workload might lead to issues due to the incorrect order of resource release by workers.
    #
    # workers  ______         _____
    #   3     /      |       /     |
    #   2    /        \     /       \     /
    #   1   /          |   /         |   /
    #   0  /            \_/           \_/
    #
    # Such workload provides guaranties that in some moment all links to shared objects (for example, in Instance Cache) will be released.
    # This may trigger certain housekeeping operations in end script's life.
    # The purpose of following test is to ensure that the order of these cleanup operations is safe and correct.
    def test_specific_workload_and_capture_instance_cache_element(self):
        # Need to retry a lot
        for _ in range(50):
            responses = []
            # "bundled" workload simulation
            with ThreadPoolExecutor(max_workers=16) as async_executor:
                for _ in range(16):
                    responses.append(
                        async_executor.submit(self.kphp_server.http_post, json=[{"op": "capture_instance_cache_element"}])
                    )

            for r in responses:
                self.assertEqual(r.result().status_code, 200)
            time.sleep(0.05)
