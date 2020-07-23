from multiprocessing.dummy import Pool as ThreadPool
import json

from python.lib.testcase import KphpServerAutoTestCase
from python.lib.engine import Engine
from python.lib.file_utils import search_engine_bin


class KphpWithConfdata(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.pmemcached = Engine(
            engine_bin=search_engine_bin("pmemcached-ram"),
            working_dir=cls.kphp_server_working_dir
        )
        cls.pmemcached.create_binlog()
        cls.pmemcached.start()

        cls.kphp_server.update_options({
            "--confdata-binlog": cls.pmemcached.binlog_path
        })

        with ThreadPool() as pool:
            pool.map(cls._store_confdata_element, cls.CONFDATA.items())

    @classmethod
    def extra_class_teardown(cls):
        cls.pmemcached.stop()

    @classmethod
    def _store_confdata_element(cls, key_value):
        cls.pmemcached.rpc_request(
            ("memcache.set", [
                ("key", key_value[0]),
                ("flags", 0),
                ("delay", 0),
                ("value", key_value[1])
            ]))

    def _call_function(self, function, param):
        resp = self.kphp_server.http_get("/{}?{}".format(function, param))
        self.assertEqual(resp.status_code, 200)
        return json.loads(resp.text)["result"]
