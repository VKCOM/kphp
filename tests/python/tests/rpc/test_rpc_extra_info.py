import json

from python.lib.testcase import KphpServerAutoTestCase


class TestRpcExtraInfo(KphpServerAutoTestCase):
    def test_untyped_rpc_extra_info(self):
        rpc_extra_info = self.kphp_server.http_get(
            "/get_kphp_untyped_rpc_extra_info?master-port={}".format(self.kphp_server.master_port))

        self.assertEqual(rpc_extra_info.status_code, 200)
        self.assertNotEqual(rpc_extra_info.text, "")

        output = json.loads(rpc_extra_info.text)

        self.assertNotEqual(output["result"], "")

        req_extra_info_arr = output["requests_extra_info"]

        self.assertEqual(len(req_extra_info_arr), 3)
        for extra_info in req_extra_info_arr:
            self.assertNotEqual(extra_info[0], 0)

        resp_extra_info_arr = output["responses_extra_info"]
        self.assertEqual(len(resp_extra_info_arr), 3)
        for extra_info in resp_extra_info_arr:
            self.assertNotEqual(extra_info[0], 0)
            self.assertNotEqual(extra_info[1], 0)

    def test_typed_rpc_extra_info(self):
        rpc_extra_info = self.kphp_server.http_get(
            "/get_kphp_typed_rpc_extra_info?master-port={}".format(self.kphp_server.master_port))

        self.assertEqual(rpc_extra_info.status_code, 200)
        self.assertNotEqual(rpc_extra_info.text, "")

        output = json.loads(rpc_extra_info.text)

        req_extra_info_arr = output["requests_extra_info"]

        self.assertEqual(len(req_extra_info_arr), 3)
        for extra_info in req_extra_info_arr:
            self.assertNotEqual(extra_info[0], 0)

        resp_extra_info_arr = output["responses_extra_info"]
        self.assertEqual(len(resp_extra_info_arr), 3)
        for extra_info in resp_extra_info_arr:
            self.assertNotEqual(extra_info[0], 0)
            self.assertNotEqual(extra_info[1], 0)
