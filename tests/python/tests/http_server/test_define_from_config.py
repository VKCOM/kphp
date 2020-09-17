from python.lib.testcase import KphpServerAutoTestCase


class TestDefineFromConfig(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--define-from-config": "data/php_ini.conf"
        })

    def test_define_from_config(self):
        resp = self.kphp_server.http_get("/ini_get?a")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "42")

        resp = self.kphp_server.http_get("/ini_get?b")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "53     1")

        resp = self.kphp_server.http_get("/ini_get?c")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "4")

        resp = self.kphp_server.http_get("/ini_get?d")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "hello")

        resp = self.kphp_server.http_get("/ini_get?efefe")
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.text, "qwerty")
