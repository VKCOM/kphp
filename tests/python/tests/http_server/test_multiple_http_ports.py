import pytest

from python.lib.testcase import WebServerAutoTestCase
from python.lib.port_generator import get_port


@pytest.mark.k2_skip_suite
class TestMultipleHttpPorts(WebServerAutoTestCase):
    WORKERS_NUM = 8
    PORTS_NUM = 4
    http_ports = [get_port() for _ in range(PORTS_NUM)]

    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--workers-num": cls.WORKERS_NUM,
            "--http-port": ",".join([str(port) for port in cls.http_ports]),
            "--verbosity-graceful-restart=1": True
        })

    def test_all_workers_working(self):
        total_worked_pids = set()
        for port in self.http_ports:
            worked_pids = set()
            for _ in range(20):
                resp = self.web_server.http_request(uri="/pid", http_port=port)
                self.assertEqual(resp.status_code, 200)
                self.assertTrue(resp.text.startswith("pid="))
                worked_pids.add(int(resp.text[4:]))
            self.assertEqual(len(worked_pids), self.WORKERS_NUM / self.PORTS_NUM)
            total_worked_pids.update(worked_pids)
        self.assertEqual(len(total_worked_pids), self.WORKERS_NUM)

    def test_sending_http_fds_on_graceful_restart(self):
        self.web_server.start()
        self.web_server.assert_log(
            ["Graceful restart: http fds sent successfully",
             "Graceful restart: http fds received successfully, got {} fds from old master".format(self.PORTS_NUM)],
            timeout=5)
        for port in self.http_ports:
            resp = self.web_server.http_request(uri="/", http_port=port)
            self.assertEqual(resp.status_code, 200)
            self.assertEqual(resp.text, "Hello world!")
