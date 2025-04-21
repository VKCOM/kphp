import json
import re
import time

from .engine import Engine
from .http_client import send_http_request, send_http_request_raw
from .port_generator import get_port


class WebServer(Engine):
    def __init__(self, web_server_bin, working_dir, options=None):
        """
        :param web_server_bin: Path to the web server binary
        :param working_dir: Working directory where the web server will be started
        :param options: Dictionary with additional options that will be used when starting the web server
            Special values:
                option: True - pass option without value
                option: None - delete default option
        """
        super(WebServer, self).__init__(web_server_bin, working_dir, options)
        self._http_port = get_port()
        self._json_log_file_read_fd = None
        self._json_log_file = None
        self._json_logs = []


    def start(self, start_msgs=None):
        super(WebServer, self).start(start_msgs)
        self._json_logs = []
        if (self._json_log_file is not None):
            self._json_log_file_read_fd = open(self._json_log_file, 'r')

    def stop(self):
        super(WebServer, self).stop()
        if self._json_log_file_read_fd:
            self._json_log_file_read_fd.close()

    @property
    def http_port(self):
        """
        :return: http port listened by web server
        """
        return self._http_port

    def http_request(self, uri='/', method='GET', http_port=None, timeout=30, **kwargs):
        """
        Send a request to the web server
        :param http_port: the port to send to, if there are multiple
        :param uri: the URI of the request
        :param method: the HTTP method of the request
        :param kwargs: additional parameters passed to the request
        :return: the response to the request
        """
        return send_http_request(self._http_port if http_port is None else http_port, uri, method, timeout, **kwargs)

    def http_post(self, uri='/', **kwargs):
        """
        Send a POST request to the web server
        :param uri: the URI of the request
        :param kwargs: additional parameters passed to the request
        :return: the response to the request
        """
        return self.http_request(uri=uri, method='POST', **kwargs)

    def http_get(self, uri='/', **kwargs):
        """
        Send a GET request to the web server
        :param uri: the URI of the request
        :param kwargs: additional parameters passed to the request
        :return: the response to the request
        """
        return self.http_request(uri=uri, method='GET', **kwargs)

    def http_request_raw(self, request):
        """
        Send a raw request to the kphp server
        :param request: a list of raw byte strings that will be concatenated into the request
        :return: the response to the request
        """
        return send_http_request_raw(self._http_port, request)

    def _read_new_json_logs(self):
        new_json_logs = list(filter(None, self._json_log_file_read_fd.readlines()))
        if new_json_logs:
            print("\n=========== Got new json logs ============")
            print("".join(new_json_logs).strip())
            print("============================================")
            for log_record in map(json.loads, new_json_logs):
                self._json_logs.append(self._process_json_log(log_record))

    def _process_json_log(self, log_record):
        return log_record

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
