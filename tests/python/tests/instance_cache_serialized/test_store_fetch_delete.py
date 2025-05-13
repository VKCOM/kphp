from python.lib.testcase import WebServerAutoTestCase

import random
from multiprocessing.dummy import Pool as ThreadPool

class TestStoreFetchDeleteSerialized(WebServerAutoTestCase):
    def test_store_fetch_delete_serialized(self):
        elements_cnt = 20

        keys = ["key{}".format(i) for i in range(elements_cnt)]
        commands = ["/store", "/fetch_and_verify", "/delete"]
        weights = [4, 20, 1]

        def worker(req_cnt):
            for _ in range(req_cnt):
                key = random.choice(keys)
                command = random.choices(commands, weights=weights, k=1)[0]
                resp = self.web_server.http_post(
                    uri=command,
                    json={"key": key})
                self.assertEqual(resp.status_code, 200)
                if command == "/store":
                    self.assertEqual(resp.json(), {"result": True})
                elif command == "/fetch_and_verify":
                    self.assertEqual(resp.json(), {"is_valid": True})

        threads_cnt = 10
        reqs_cnt = 75
        workers_cnt = 20
        with ThreadPool(threads_cnt) as pool:
            for _ in pool.map(worker, [reqs_cnt for _ in range(workers_cnt)]):
                pass
