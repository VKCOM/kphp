import os
import time

from .web_server import WebServer


class K2Server(WebServer):
    def __init__(self, k2_server_bin, working_dir, kphp_build_dir, options=None, auto_start=False):
        """
        :param k2_server_bin: Путь до бинарника k2 сервера
        :param working_dir: Рабочая папка где будет запущен k2 сервер
        :param options: Словарь с дополнительными опциями, которые будут использоваться при запуске kphp сервера
            Специальная значения:
                option: True - передавать опцию без значения
                option: None - удалить дефолтно проставляемую опцию
        """
        super(K2Server, self).__init__(k2_server_bin, working_dir, options)
        self._images_dir = kphp_build_dir
        self._linking_file = os.path.join(working_dir, "data/component-config.yaml")
        self._options = {"start-node": True,
                         "--host": "127.0.0.1",
                         "--port": self.http_port,
                         "--images-dir": self._images_dir,
                         "--linking": self._linking_file,
                         "--log-file": self._json_log_file}

        if options:
            self.update_options(options)
        if auto_start:
            self.start()

    def start(self, start_msgs=None):
        super(K2Server, self).start(start_msgs)

    def stop(self):
        super(K2Server, self).stop()

    def set_error_tag(self, tag):
        error_tag_file = None
        if tag is not None:
            error_tag_file = os.path.join(self._working_dir, "error_tag_file")
            with open(error_tag_file, 'w') as f:
                f.write(str(tag))
        self.update_options({"--error-tag": error_tag_file})

