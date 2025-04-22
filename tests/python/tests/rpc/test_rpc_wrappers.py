import json
import typing
import pytest

from python.lib.kphp_server import KphpServer
from python.lib.testcase import WebServerAutoTestCase

BAD_ACTOR_ID_ERROR_CODE = -2002
WRONG_QUERY_ID_ERROR_CODE = -1003


@pytest.mark.k2_skip_suite
class TestRpcWrappers(WebServerAutoTestCase):
    def test_rpc_no_wrappers_with_actor_id(self):
        rpc_response = self.web_server.http_get("/test_rpc_no_wrappers_with_actor_id?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], BAD_ACTOR_ID_ERROR_CODE)

    def test_rpc_no_wrappers_with_ignore_answer(self):
        rpc_response = self.web_server.http_get("/test_rpc_no_wrappers_with_ignore_answer?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], WRONG_QUERY_ID_ERROR_CODE)

    def test_rpc_dest_actor_with_actor_id(self):
        rpc_response = self.web_server.http_get("/test_rpc_dest_actor_with_actor_id?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], BAD_ACTOR_ID_ERROR_CODE)

    def test_rpc_dest_actor_with_ignore_answer(self):
        rpc_response = self.web_server.http_get("/test_rpc_dest_actor_with_ignore_answer?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], WRONG_QUERY_ID_ERROR_CODE)

    def test_rpc_dest_flags_with_actor_id(self):
        bad_actor_id_error_code = -2002
        rpc_response = self.web_server.http_get("/test_rpc_dest_flags_with_actor_id?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], BAD_ACTOR_ID_ERROR_CODE)

    def test_rpc_dest_flags_with_ignore_answer(self):
        rpc_response = self.web_server.http_get("/test_rpc_dest_flags_with_ignore_answer?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], WRONG_QUERY_ID_ERROR_CODE)

    def test_rpc_dest_flags_with_ignore_answer_1(self):
        rpc_response = self.web_server.http_get("/test_rpc_dest_flags_with_ignore_answer_1?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], WRONG_QUERY_ID_ERROR_CODE)

    def test_rpc_dest_actor_flags_with_actor_id(self):
        rpc_response = self.web_server.http_get("/test_rpc_dest_actor_flags_with_actor_id?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], BAD_ACTOR_ID_ERROR_CODE)

    def test_rpc_dest_actor_flags_with_ignore_answer(self):
        rpc_response = self.web_server.http_get("/test_rpc_dest_actor_flags_with_ignore_answer?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], WRONG_QUERY_ID_ERROR_CODE)

    def test_rpc_dest_actor_flags_with_ignore_answer_1(self):
        rpc_response = self.web_server.http_get("/test_rpc_dest_actor_flags_with_ignore_answer_1?master-port={}".format(typing.cast(KphpServer, self.web_server).master_port))

        self.assertEqual(rpc_response.status_code, 200)
        self.assertNotEqual(rpc_response.text, "")

        output = json.loads(rpc_response.text)

        self.assertEqual(output["error"], WRONG_QUERY_ID_ERROR_CODE)
