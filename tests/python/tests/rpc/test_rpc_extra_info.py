import json

from python.lib.testcase import KphpServerAutoTestCase


class TestRpcExtraInfo(KphpServerAutoTestCase):
    def test_untyped_rpc_extra_info(self):
        rpc_extra_info = self.kphp_server.http_get(
            "/test_kphp_untyped_rpc_extra_info?master-port={}".format(self.kphp_server.master_port))

        self.assertEqual(rpc_extra_info.status_code, 200)
        self.assertNotEqual(rpc_extra_info.text, "")

        output = json.loads(rpc_extra_info.text)

        self.assertNotEqual(output["result"], "")

        req_extra_info_arr = output["requests_extra_info"]

        requests_size_arr = [28, 28, 28, 32]
        self.assertEqual(len(req_extra_info_arr), 4)
        for i, extra_info in enumerate(req_extra_info_arr):
            self.assertEqual(extra_info[0], requests_size_arr[i])

        resp_extra_info_arr = output["responses_extra_info"]
        self.assertEqual(len(resp_extra_info_arr), 4)
        for extra_info in resp_extra_info_arr:
            self.assertNotEqual(extra_info[0], 0)
            self.assertNotEqual(extra_info[1], 0)

        # check engine response time using extra info of engine.sleep request
        self.assertTrue(resp_extra_info_arr[-1][1] > 0.2)

    def test_typed_rpc_extra_info(self):
        rpc_extra_info = self.kphp_server.http_get(
            "/test_kphp_typed_rpc_extra_info?master-port={}".format(self.kphp_server.master_port))

        self.assertEqual(rpc_extra_info.status_code, 200)
        self.assertNotEqual(rpc_extra_info.text, "")

        output = json.loads(rpc_extra_info.text)

        req_extra_info_arr = output["requests_extra_info"]

        requests_size_arr = [28, 28, 28, 32]
        self.assertEqual(len(req_extra_info_arr), 4)
        for i, extra_info in enumerate(req_extra_info_arr):
            self.assertEqual(extra_info[0], requests_size_arr[i])

        resp_extra_info_arr = output["responses_extra_info"]
        self.assertEqual(len(resp_extra_info_arr), 4)
        for extra_info in resp_extra_info_arr:
            self.assertNotEqual(extra_info[0], 0)
            self.assertNotEqual(extra_info[1], 0)

        # check engine response time using extra info of engine.sleep request
        self.assertTrue(resp_extra_info_arr[-1][1] > 0.2)
