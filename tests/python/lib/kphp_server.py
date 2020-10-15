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
        self._options["--cluster-name"] = \
            "kphp_server_{}".format("".join(chr(ord('A') + int(c)) for c in str(self._http_port)))
        self._options["--disable-sql"] = True
        self._options["--workers-num"] = 2
        if options:
            self.update_options(options)
        if auto_start:
            self.start()

    @property
    def master_port(self):
        """
        :return: port listened by master process
        """
        return self._master_port

    def http_request(self, uri='/', method='GET', **kwargs):
        """
        Послать запрос в kphp сервер
        :param uri: uri запроса
        :param method: метод запроса
        :param kwargs: дополнительные параметры, передаваемые в request
        :return: ответ на запрос
        """
        return send_http_request(self._http_port, uri, method, **kwargs)

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
