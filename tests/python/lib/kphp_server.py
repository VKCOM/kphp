import os
import time

from .port_generator import get_port
from .web_server import WebServer


class KphpServer(WebServer):
    def __init__(self, kphp_server_bin, working_dir, options=None, auto_start=False):
        """
        :param kphp_server_bin: Path to the kphp server binary
        :param working_dir: Working directory where the kphp server will be started
        :param options: Dictionary with additional options that will be used when starting the kphp server
            Special values:
                option: True - pass the option without a value
                option: None - remove the default option
        """
        super(KphpServer, self).__init__(kphp_server_bin, working_dir, options)
        self._options["--port"] = None
        self._master_port = get_port()
        self._options["--http-port"] = self._http_port
        self._options["--master-port"] = self._master_port
        self._options["--master-name"] = \
            "kphp_server_{}".format("".join(chr(ord('A') + int(c)) for c in str(self._http_port)))
        self._options["--server-config"] = self.get_server_config_path()
        self._options["--workers-num"] = 1
        self._options["--allow-loopback"] = None
        self._options["--dump-next-queries"] = None
        self._json_log_file = self._log_file + ".json"
        if options:
            self.update_options(options)
        if auto_start:
            self.start()

    def start(self, start_msgs=None):
        with open(self.get_server_config_path(), mode="w+t", encoding="utf-8") as fd:
            fd.write(
                "cluster_name: kphp_server_{}".format("".join(chr(ord('A') + int(c)) for c in str(self._http_port))))
        super(KphpServer, self).start(start_msgs)

    def stop(self):
        super(KphpServer, self).stop()
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

    def get_workers(self):
        return self._engine_process.children()

    def get_server_config_path(self):
        return os.path.join(self._working_dir, "server_config.yml")

    def _process_json_log(self, log_record):
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
        return log_record
