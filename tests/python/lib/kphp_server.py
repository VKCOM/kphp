import requests
import socket
from requests_toolbelt.utils import dump

from .engine import Engine
from .port_generator import get_port
from .colors import blue


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
        self._http_port = get_port()
        self._options["--http-port"] = self._http_port
        self._options["--cluster-name"] = \
            "kphp_server_{}".format("".join(chr(ord('A') + int(c)) for c in str(self._http_port)))
        self._options["--disable-sql"] = True
        self._options["--workers-num"] = 2
        if options:
            self.update_options(options)
        if auto_start:
            self.start()

    def http_request(self, uri='/', method='GET', **kwargs):
        """
        Послать запрос в kphp сервер
        :param uri: uri запроса
        :param method: метод запроса
        :param kwargs: дополнительные параметры, передаваемые в request
        :return: ответ на запрос
        """
        session = requests.session()
        url = 'http://127.0.0.1:{}{}'.format(self._http_port, uri)
        print("\nSending HTTP request: [{}]".format(blue(url)))
        r = session.request(method=method, url=url, timeout=30, **kwargs)
        session.close()
        print("HTTP request debug:")
        print("=============================")
        print(*[i for i in dump.dump_all(r).splitlines(True)], sep="\n")
        print("=============================")
        return r

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
        crlf = b"\r\n"
        msg = (crlf.join(request) + crlf * 2)

        print("\nSending Raw HTTP request")
        print("=============================")
        print(*msg.splitlines(True), sep="\n")
        print("=============================")

        s = socket.socket()
        s.connect(('127.0.0.1', self._http_port))
        s.send(msg)
        response_bytes = b''
        buffer_size = 4096
        buffer = s.recv(buffer_size)
        response_bytes += buffer

        while buffer and len(buffer) == buffer_size:
            buffer = s.recv(buffer_size)
            response_bytes += buffer

        s.close()
        print("\nGot Raw HTTP response")
        print("=============================")
        print(*response_bytes.splitlines(True), sep="\n")
        print("=============================")

        class _RawResponse:
            def __init__(self, raw_bytes):
                self.raw_bytes = raw_bytes
                head, _, body = raw_bytes.partition(b"\r\n\r\n")

                first_line, _, headers_data = head.partition(b"\r\n")
                self.method, _, status = first_line.partition(b' ')
                status_code, _, status_line = status.partition(b' ')
                self.status_code = int(status_code)
                self.reason = status_line
                self.headers = {}
                self.content = body

                for i in headers_data.splitlines():
                    k, v = i.split(b': ')
                    self.headers[k.decode()] = v.decode()

        return _RawResponse(response_bytes)
