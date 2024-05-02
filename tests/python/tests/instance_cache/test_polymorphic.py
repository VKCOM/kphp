from python.lib.testcase import KphpServerAutoTestCase


class TestPolymorphic(KphpServerAutoTestCase):

    def test_store_fetch_modify(self):
        resp = self.kphp_server.http_post(
            uri='/polymorphic_store'
        )
        self.assertEqual(resp.status_code, 200)
        self.kphp_server.assert_log(["successfully store SimpleType", "successfully store ComplexType", "successfully store CompletelyComplexType"])
        for i in range(10):
            resp = self.kphp_server.http_post(
                uri='/polymorphic_store_and_verify'
            )
            self.assertEqual(resp.status_code, 200)
            self.kphp_server.assert_log(["successfully fetch SimpleType", "successfully fetch ComplexType", "successfully fetch CompletelyComplexType"])
