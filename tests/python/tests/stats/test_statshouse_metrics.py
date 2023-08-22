from multiprocessing.dummy import Pool as ThreadPool
from python.lib.testcase import KphpServerAutoTestCase


class TestStatshouseMetrics(KphpServerAutoTestCase):
    WORKERS_NUM = 2

    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": cls.WORKERS_NUM,
            "--statshouse-client": ":" + str(cls.kphp_server._statshouse.port),
        })

    def test_sending_metrics(self):
        self.kphp_server.stop()
        self.kphp_server.start(None, True)
        self.kphp_server.check_statshouse_metrics()
