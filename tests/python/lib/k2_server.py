import os
import time

from .web_server import WebServer


class K2Server(WebServer):
    def __init__(self, k2_server_bin, working_dir, kphp_build_dir, options=None, auto_start=False):
        """
        :param k2_server_bin: Путь до бинарника k2 сервера
        :param working_dir: Рабочая папка где будет запущен k2 сервер
        :param kphp_build_dir: Директория с кодом компонент
        :param options: Словарь с дополнительными опциями, которые будут использоваться при запуске kphp сервера
            Специальная значения:
                option: True - передавать опцию без значения
                option: None - удалить дефолтно проставляемую опцию
        """
        super(K2Server, self).__init__(k2_server_bin, working_dir, options)
        self._images_dir = kphp_build_dir
        self._working_dir = working_dir
        self._linking_file = os.path.join(working_dir, "data/component-config.yaml")
        self._json_log_file = None
        self._options = {"start-node": True,
                         "--host": "127.0.0.1",
                         "--port": self.http_port,
                         "--images-dir": self._images_dir,
                         "--linking": self._linking_file}

        if options:
            self.update_options(options)
        if auto_start:
            self.start()

    def start(self, start_msgs=None):
        if "--log-file" in self._options:
            self._json_log_file = os.path.join(self._working_dir, self._options["--log-file"])
        else:
            if start_msgs is None:
                start_msgs = []
            start_msgs.append("Starting to accept clients.")
        super(K2Server, self).start(start_msgs)

    def stop(self):
        super(K2Server, self).stop()

