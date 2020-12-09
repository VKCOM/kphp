import signal
import resource

from python.lib.testcase import KphpServerAutoTestCase


class TestJsonLogsSignals(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.ignore_log_errors()
        cls.kphp_server.update_options({
            "--workers-num": 5
        })
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

    def test_sigsegv_from_code(self):
        with self.assertRaises(Exception):
            self.kphp_server.http_post(
                json=[
                    {"op": "set_context", "env": "e1", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                    {"op": "sigsegv"}
                ])

        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": -1, "env": "e1", "msg": "SIGSEGV terminating program",
                "tags": {"a": "b", "uncaught": True}, "extra_info": {"c": "d"}
            }])

    def test_worker_sigsegv(self):
        self.kphp_server.get_workers()[0].send_signal(signal.SIGSEGV)
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": -1, "env": "", "msg": "SIGSEGV terminating program",
                "tags": {"uncaught": True}
            }])

    def test_worker_sigbus(self):
        self.kphp_server.get_workers()[1].send_signal(signal.SIGBUS)
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": -1, "env": "", "msg": "SIGBUS terminating program",
                "tags": {"uncaught": True}
            }])

    def test_worker_sigabrt(self):
        self.kphp_server.get_workers()[2].send_signal(signal.SIGABRT)
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": -1, "env": "", "msg": "SIGABRT terminating program",
                "tags": {"uncaught": True}
            }])

    def test_stack_overflow(self):
        res = self.kphp_server.http_post(
            json=[
                {"op": "set_context", "env": "e1", "tags": {"a": "b"}, "extra_info": {"c": "d"}},
                {"op": "stack_overflow"}
            ])
        self.assertEqual(res.status_code, 500)
        self.assertEqual(res.text, "ERROR")

        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": 1, "env": "e1", "msg": "Stack overflow",
                "tags": {"a": "b", "uncaught": True}, "extra_info": {"c": "d"}
            }])

    def test_master_sigsegv(self):
        self.kphp_server.send_signal(signal.SIGSEGV)
        with self.assertRaises(RuntimeError):
            self.kphp_server.stop()
        self.kphp_server.start()
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": -1, "env": "", "msg": "SIGSEGV terminating program",
                "tags": {"uncaught": True}
            }])

    def test_master_sigbus(self):
        self.kphp_server.send_signal(signal.SIGBUS)
        with self.assertRaises(RuntimeError):
            self.kphp_server.stop()
        self.kphp_server.start()
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": -1, "env": "", "msg": "SIGBUS terminating program",
                "tags": {"uncaught": True}
            }])

    def test_master_sigabrt(self):
        self.kphp_server.send_signal(signal.SIGABRT)
        self.kphp_server.restart()
        self.kphp_server.assert_json_log(
            expect=[{
                "version": 0, "type": -1, "env": "", "msg": "SIGABRT terminating program",
                "tags": {"uncaught": True}
            }])
