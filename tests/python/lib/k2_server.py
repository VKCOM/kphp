import os
import time

from .web_server import WebServer


class K2Server(WebServer):
    def __init__(self, k2_server_bin, working_dir, kphp_build_dir, options=None, auto_start=False):
        """
        :param k2_server_bin: Path to the k2 server binary
        :param working_dir: Working directory where the k2 server will be started
        :param kphp_build_dir: Directory with compiled k2-components
        :param options: Dictionary with additional options that will be used when starting the k2 server
            Special values:
                option: True - pass option without value
                option: None - delete default option
        """
        super(K2Server, self).__init__(k2_server_bin, working_dir, options)
        self._images_dir = kphp_build_dir
        self._working_dir = working_dir
        self._linking_file = os.path.join(working_dir, "data/component-config.yaml")
        self._json_log_file = None
        self._options = {"start-node": True,
                         "--host": "127.0.0.1",
                         "--http-ports": self.http_port,
                         "--rpc-ports": self.rpc_port,
                         "--images-dir": self._images_dir,
                         "--restart-socket": "/tmp/k2_restart_node_{}".format(hash(self._working_dir)),
                         "--linking": self._linking_file}

        if options:
            self.update_options(options)
        if auto_start:
            self.start()

    def start(self, start_msgs=None):
        if self._is_json_log_enabled():
            self._json_log_file = os.path.join(self._working_dir, self._options["--log-file"])
        else:
            start_msgs = start_msgs or []
            start_msgs.append("Starting to accept clients.")
        super(K2Server, self).start(start_msgs)

    def stop(self):
        super(K2Server, self).stop()

    def _is_json_log_enabled(self):
        return "--log-file" in self._options

