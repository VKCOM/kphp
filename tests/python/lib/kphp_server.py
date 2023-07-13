import json
import time
import re
import os

from .engine import Engine
from .port_generator import get_port
from .http_client import send_http_request, send_http_request_raw


class KphpServer(Engine):
    def __init__(self, engine_bin, working_dir, options=None, auto_start=False):
        """
        :param engine_bin: Путь до бинарника kphp сервера
        :param working_dir: Рабочая папка где будет запущен kphp сервер
        :param options: Словарь с дополнительными опциями, которые будут использоваться при запуске kphp сервера
            Специальная значения:
                option: True - передавать опцию без значения
                option: None - удалить дефолтно проставляемую опцию
        """
        super(KphpServer, self).__init__(engine_bin, working_dir)
        self._options["--port"] = None
        self._http_port = get_port()
        self._master_port = get_port()
        self._options["--http-port"] = self._http_port
        self._options["--master-port"] = self._master_port
        self._options["--master-name"] = \
            "kphp_server_{}".format("".join(chr(ord('A') + int(c)) for c in str(self._http_port)))
        self._options["--server-config"] = self.get_server_config_path()
        self._options["--disable-sql"] = True
        self._options["--workers-num"] = 1
        self._options["--allow-loopback"] = None
        self._options["--dump-next-queries"] = None
        self._json_log_file_read_fd = None
        self._json_logs = []
        if options:
            self.update_options(options)
        if auto_start:
            self.start()

    def start(self, start_msgs=None):
        with open(self.get_server_config_path(), mode="w+t", encoding="utf-8") as fd:
            fd.write(
                "cluster_name: kphp_server_{}".format("".join(chr(ord('A') + int(c)) for c in str(self._http_port))))
        super(KphpServer, self).start(start_msgs)
        self._json_logs = []
        self._json_log_file_read_fd = open(self._log_file + ".json", 'r')

    def stop(self):
        super(KphpServer, self).stop()
        if self._json_log_file_read_fd:
            self._json_log_file_read_fd.close()
        if os.path.exists(self.get_server_config_path()):
            os.remove(self.get_server_config_path())

    def set_error_tag(self, tag):
        error_tag_file = None
        if tag is not None:
            error_tag_file = os.path.join(self._working_dir, "error_tag_file")
            with open(error_tag_file, 'w') as f:
                f.write(str(tag))
        self.update_options({"--error-tag": error_tag_file})

    @property
    def master_port(self):
        """
        :return: port listened by master process
        """
        return self._master_port

    @property
    def http_port(self):
        """
        :return: http port listened by workers
        """
        return self._http_port

    def http_request(self, uri='/', method='GET', http_port=None, timeout=30, **kwargs):
        """
        Послать запрос в kphp сервер
        :param http_port: порт в который посылать, если их несколько
        :param uri: uri запроса
        :param method: метод запроса
        :param kwargs: дополнительные параметры, передаваемые в request
        :return: ответ на запрос
        """
        return send_http_request(self._http_port if http_port is None else http_port, uri, method, timeout, **kwargs)

    def http_post(self, uri='/', **kwargs):
        """
        Послать POST запрос в kphp сервер
        :param uri: uri запроса
        :param kwargs: дополнительные параметры, передаваемые в request
        :return: ответ на запрос
        """
        return self.http_request(uri=uri, method='POST', **kwargs)

    def http_get(self, uri='/', **kwargs):
        """
        Послать GET запрос в kphp сервер
        :param uri: uri запроса
        :param kwargs: дополнительные параметры, передаваемые в request
        :return: ответ на запрос
        """
        return self.http_request(uri=uri, method='GET', **kwargs)

    def http_request_raw(self, request):
        """
        Послать сырой запрос в kphp сервер
        :param request: список сырых байтовых строк, которые будут сконкатеннированных в запрос
        :return: ответ на запрос
        """
        return send_http_request_raw(self._http_port, request)

    def _read_new_json_logs(self):
        new_json_logs = list(filter(None, self._json_log_file_read_fd.readlines()))
        if new_json_logs:
            print("\n=========== Got new json logs ============")
            print("".join(new_json_logs).strip())
            print("============================================")
            for log_record in map(json.loads, new_json_logs):
                trace = log_record.get("trace", [])
                if not trace:
                    raise RuntimeError("Got an empty trace in json log")
                bad_addresses = list(filter(lambda address: not address.startswith("0x"), trace))
                if bad_addresses:
                    raise RuntimeError("Got bad addresses in json log trace: " + ", ".join(bad_addresses))
                del log_record["trace"]
                created_at = log_record.get("created_at", 0)
                if abs(created_at - time.time()) > 60:
                    raise RuntimeError("Got bad timestamp in json log created_at: {}".format(created_at))
                del log_record["created_at"]
                got_tags = log_record.get("tags")
                if (got_tags.get("process_type") not in ["general_worker", "http_worker", "rpc_worker", "master"]) \
                        or (got_tags.get("logname_id") is not None and not isinstance(got_tags.get("logname_id"), int)) \
                        or (not isinstance(got_tags.get("pid"), int)):
                    raise RuntimeError("Got bad process_type tags in json log: {}".format(got_tags))
                if log_record["tags"]["process_type"] != "master":
                    del log_record["tags"]["logname_id"]
                del log_record["tags"]["process_type"]
                del log_record["tags"]["pid"]
                if not got_tags.get("cluster", ""):
                    raise RuntimeError("Got an empty cluster in json log: {}".format(got_tags))
                del log_record["tags"]["cluster"]
                self._json_logs.append(log_record)

    def assert_json_log(self, expect, message="Can't wait expected json log", timeout=60):
        """
        Check kphp server json log
        :param expect: Expected json record list
        :param message: Error message in case of failure
        :param timeout: Json records waiting time
        """
        start = time.time()
        expected_records = expect[:]

        while expected_records:
            self._assert_availability()
            self._read_new_json_logs()
            self._json_logs = list(filter(None, self._json_logs))
            for index, json_log_record in enumerate(self._json_logs):
                if not expected_records:
                    return
                expected_record = expected_records[0]
                expected_msg = expected_record["msg"]
                got_msg = json_log_record["msg"]
                if re.search(expected_msg, got_msg):
                    expected_record_copy = expected_record.copy()
                    expected_record_copy["msg"] = ""
                    json_log_record_copy = json_log_record.copy()
                    json_log_record_copy["msg"] = ""
                    if expected_record_copy == json_log_record_copy:
                        expected_records.pop(0)
                        self._json_logs[index] = None

            time.sleep(0.05)
            if time.time() - start > timeout:
                expected_str = json.dumps(obj=expected_records, indent=2)
                raise RuntimeError("{}; Missed messages: {}".format(message, expected_str))

    def get_workers(self):
        return self._engine_process.children()

    def get_server_config_path(self):
        return os.path.join(self._working_dir, "server_config.yml")
