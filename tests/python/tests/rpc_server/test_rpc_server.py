import pytest
import random
from multiprocessing.dummy import Pool as ThreadPool

from python.lib.testcase import WebServerAutoTestCase
from python.lib.tl_client import send_rpc_request


@pytest.mark.kphp_skip
class TestRpcServer(WebServerAutoTestCase):
    ACTOR_ID = 11
    BINLOG_POS = 22
    RETURN_BINLOG_POS_FLAG = 0x1
    EXPECTED_RESPONSE = 200

    @classmethod
    def get_ping_request(cls):
        return ("kphp.pingRpcCluster", [("id", 1)])

    @classmethod
    def get_dest_actor_request(cls, query):
        return ("rpcDestActor", [("actor_id", TestRpcServer.ACTOR_ID), ("query", query)])

    @classmethod
    def get_dest_flags_request(cls, query):
        return ("rpcDestFlags", [
            ("flags", TestRpcServer.RETURN_BINLOG_POS_FLAG),
            ("extra", ("rpcInvokeReqExtra", [("return_binlog_pos", "( true )")])),
            ("query", query)]
        )

    @classmethod
    def get_dest_actor_flags_request(cls, query):
        return ("rpcDestActorFlags", [
            ("actor_id", TestRpcServer.ACTOR_ID),
            ("flags", TestRpcServer.RETURN_BINLOG_POS_FLAG),
            ("extra", ("rpcInvokeReqExtra", [("return_binlog_pos", "( true )")])),
            ("query", query)]
        )

    def do_simple_request_test(self, it):
        response = self.web_server.rpc_request(TestRpcServer.get_ping_request())
        self.assertEqual(response, TestRpcServer.EXPECTED_RESPONSE)

    def do_dest_actor_request_test(self, it):
        response = self.web_server.rpc_request(TestRpcServer.get_dest_actor_request(TestRpcServer.get_ping_request()))
        self.assertEqual(response, TestRpcServer.EXPECTED_RESPONSE)

    def do_dest_flags_request_test(self, it):
        response = self.web_server.rpc_request(TestRpcServer.get_dest_flags_request(TestRpcServer.get_ping_request()))
        self.assertEqual(response, TestRpcServer.BINLOG_POS)

    def do_dest_actor_flags_request_test(self, it):
        response = self.web_server.rpc_request(TestRpcServer.get_dest_actor_flags_request(TestRpcServer.get_ping_request()))
        self.assertEqual(response, TestRpcServer.BINLOG_POS)

    # def do_test_multiple_headers(self, it): TODO enable once k2-node starts accepting multiple headers
    #     response = self.web_server.rpc_request(
    #         TestRpcServer.get_dest_flags_request(TestRpcServer.get_dest_actor_request(TestRpcServer.get_ping_request())))
    #     self.assertEqual(response, TestRpcServer.BINLOG_POS)

    def do_random_test(self, it):
        tests = [
            self.do_simple_request_test,
            self.do_dest_actor_request_test,
            self.do_dest_flags_request_test,
            self.do_dest_actor_flags_request_test,
            # self.do_test_multiple_headers,
        ]
        random.choice(tests)(it)

    # def test_unexpected_request(self): TODO once when tlclient starts parsing new error format
    #     response = self.web_server.rpc_request(("rpcPing", [("ping_id", 1),]))
    #     self.assertEqual(response, 200)

    def test_requests(self):
        requests_count = 1000
        with ThreadPool(5) as pool:
            for _ in pool.imap_unordered(self.do_random_test, range(requests_count)):
                pass

