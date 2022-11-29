
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <functional>
#include <sys/poll.h>

#include "runtime/net_events.h"
#include "runtime/critical_section.h"
#include "runtime/datetime/datetime_functions.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"
#include "runtime/tcp.h"

namespace {

int DEFAULT_SOCKET_TIMEOUT = 60;

int opened_tcp_client_sockets_last_query_nam = -1;
char opened_tcp_sockets_storage[sizeof(array<int>)];
array<FILE *> * opened_tcp_client_sockets = reinterpret_cast <array<FILE *> *> (opened_tcp_sockets_storage);
addrinfo tcp_hints = {0, AF_UNSPEC, SOCK_STREAM, 0, 0, nullptr, nullptr, nullptr};

namespace details {
Optional<std::pair<string, int64_t>> parse_url(const string &url,
                                               const std::function<void(int64_t, const string &, string)> &faulter) {
  string url_to_parse = url;
  if (!url_to_parse.starts_with(string("tcp://"))) {
    faulter(-7, string("\"%s\" doesn't start with tcp"), url_to_parse);
    return {};
  }
  url_to_parse = url_to_parse.substr(6, url_to_parse.size() - 6);
  string::size_type pos = url_to_parse.find(string(":", 1));
  if (pos == string::npos) {
    faulter(-7, string("Failed to parse address \"%s\""), url_to_parse);
    return {};
  }
  string host = url_to_parse.substr(0, pos);
  int64_t port = f$intval(url_to_parse.substr(pos + 1, url_to_parse.size() - (pos + 1)));

  if (host.empty()) {
    faulter(-1, string("Wrong host specified in url \"%s\""), url_to_parse);
    return {};
  }

  if (port <= 0 || port >= 65536) {
    faulter(-2, string("Wrong port specified in url \"%s\""), url_to_parse);
    return {};
  }

  return std::pair<string, int64_t>(host, port);
}

Optional<int64_t> connect_to_address(const string &host, int64_t port, double end_time,
                                     const std::function<void(int64_t, const string &, string)> &faulter) {
  addrinfo *result = nullptr;
  int get_result = getaddrinfo(host.c_str(), string(port).c_str(), &tcp_hints, &result); /*allocation */
  if (get_result != 0 || result == nullptr) {
    faulter(-3, string("Can't resolve host \"%s\""), host);
    return {};
  }
  /* result points to an allocated dynamically linked list of
   * addrinfo structures linked by the ai_next component.
   * There are several reasons for this. We will use the first element */
  addrinfo * rp = result;
  int64_t socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
  if (socket_fd == -1) {
    faulter(-3, string("Can't create tcp socket. System call 'socket(...)' got error"), string(strerror(errno)));
    freeaddrinfo(result);
    return {};
  }
  fcntl(socket_fd, F_SETFL, O_NONBLOCK);

  if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != 0) {
    if (errno != EINPROGRESS) {
      faulter(-3, string("Can't connect to tcp socket. System call 'connect(...)' got error"), string(strerror(errno)));
      freeaddrinfo(result);
      return {};
    }
  }

  pollfd poll_fd;
  poll_fd.fd = socket_fd;
  poll_fd.events = POLLIN | POLLERR | POLLHUP | POLLOUT | POLLPRI;

  double timeout = end_time - microtime_monotonic();
  if (timeout <= 0) {
    faulter(-3, string("Timeout expired %s"), string(""));
    freeaddrinfo(result);
    return {};
  } else if (poll(&poll_fd, 1, timeout_convert_to_ms(timeout)) <= 0) {
    faulter(-3, string("Can't connect to tcp socket. System call 'poll(...)' got error"), string(strerror(errno)));
    freeaddrinfo(result);
    return {};
  } else {
    freeaddrinfo(result);
    int old_flags = fcntl(socket_fd, F_GETFL);
    fcntl(socket_fd, F_SETFL, old_flags & ~O_NONBLOCK);
    return socket_fd;
  }
}

void register_socket(const Stream & stream) {
  if (dl::query_num != opened_tcp_client_sockets_last_query_nam) {
    new(opened_tcp_sockets_storage) array<int>();
    opened_tcp_client_sockets_last_query_nam = dl::query_num;
  }
  opened_tcp_client_sockets->set_value(stream, nullptr);
}
} // namespace details

Stream tcp_stream_socket_client(const string &url, int64_t &error_number, string &error_description, double timeout,
                                int64_t flags __attribute__((unused)),
                                const mixed &context __attribute__((unused))) {
  if (timeout < 0) {
    timeout = DEFAULT_SOCKET_TIMEOUT;
  }

  double end_time = microtime_monotonic() + timeout;
  auto set_format_error = [&](int64_t error_no, const string & format, string param) {
    error_number = error_no;
    error_description = f$sprintf(format, array<mixed>::create(std::move(param)));
  };

  auto info = details::parse_url(url, set_format_error);
  if (!info.has_value()) {
    php_warning("stream_socket_client() : Unable to connect to %s (%s)", url.c_str(), error_description.c_str());
    return false;
  }

  dl::CriticalSectionSmartGuard guard;
  Optional<int64_t> fd{};
  if (flags & STREAM_CLIENT_CONNECT) {
    fd = details::connect_to_address(info.val().first, info.val().second, end_time, set_format_error);
  }
  if (!fd.has_value()) {
    php_warning("stream_socket_client() : Unable to connect to %s (%s)", url.c_str(), error_description.c_str());
    return false;
  }

  Stream stream = url;
  stream.append(string("-fd-")).append(f$strval(fd.val()));
  details::register_socket(stream);
  return stream;
}

int tcp_get_fd(const Stream &stream) {
  if (dl::query_num != opened_tcp_client_sockets_last_query_nam || !opened_tcp_client_sockets->has_key(stream.to_string())) {
    php_warning("TCP socket \"%s\" is not opened", stream.to_string().c_str());
    return -1;
  }

  string::size_type pos = stream.to_string().size() - 1;
  while (stream.to_string()[pos] != '-') {
    --pos;
  }
  int fd = f$intval(stream.to_string().substr(pos + 1, stream.to_string().size() - (pos + 1)));
  return fd;
}

Optional<int64_t> tcp_fwrite(const Stream &stream, const string &data) {
  int fd = tcp_get_fd(stream);
  if (fd == -1) {
    return {};
  }

  dl::enter_critical_section();
  int res = send(fd, data.c_str(), data.size(), 0);
  dl::leave_critical_section();

  if (res == -1) {
    if (errno != EAGAIN) {
      php_warning("fwrite() : An error occurred while sending TCP-package");
      return false;
    } else {
      return 0;
    }
  }
  return res;
}

Optional<string> tcp_fread(const Stream &stream, int64_t length) {
  int fd = tcp_get_fd(stream);
  if (fd == -1) {
    return {};
  }
  string data(length, false);

  dl::enter_critical_section();
  int res = recv(fd, data.buffer(), length, 0);
  dl::leave_critical_section();

  if (res == -1) {
    if (errno != EAGAIN) {
      php_warning("fread() : An error occurred while receiving TCP-package");
      return {};
    } else {
      return "";
    }
  }
  return data.substr(0, res);
}

Optional<string> tcp_fgets(const Stream &stream, int64_t length) {
  int fd = tcp_get_fd(stream);
  if (fd == -1) {
    return false;
  } else if (length < 0) {
    length = INT_MAX;
  }
  string data(length, false);

  dl::enter_critical_section();
  if (opened_tcp_client_sockets->get_value(stream) == nullptr) {
    opened_tcp_client_sockets->set_value(stream, fdopen(fd, "r"));
  }
  FILE * src = opened_tcp_client_sockets->get_value(stream);
  char * res = fgets(data.buffer(), length, src);
  dl::leave_critical_section();

  if (res == nullptr) {
    return false;
  }
  string::size_type end_pos = data.find(string("\n")) + 1;
  return data.substr(0, end_pos);
}

Optional<string> tcp_fgetc(const Stream & stream) {
  int fd = tcp_get_fd(stream);
  if (fd == -1) {
    return false;
  }

  dl::enter_critical_section();
  if (!opened_tcp_client_sockets->get_value(stream)) {
    opened_tcp_client_sockets->set_value(stream, fdopen(fd, "r"));
  }
  FILE * src = opened_tcp_client_sockets->get_value(stream);
  int res = fgetc(src);
  dl::leave_critical_section();

  if (res == -1) {
    return false;
  }
  return f$chr(res);
}

bool tcp_fclose(const Stream &stream) {
  int fd = tcp_get_fd(stream);

  dl::enter_critical_section();
  int res = close(fd);
  if (opened_tcp_client_sockets->get_value(stream)) {
    fclose(opened_tcp_client_sockets->get_value(stream));
  }
  opened_tcp_client_sockets->unset(stream);
  dl::leave_critical_section();

  if (res == -1) {
    php_warning("fclose() : An error occurred while closing TCP-package");
    return false;
  }
  return true;
}

bool tcp_feof(const Stream &stream) {
  int fd = tcp_get_fd(stream);
  if (fd == -1) {
    return false;
  }

  dl::enter_critical_section();
  if (!opened_tcp_client_sockets->get_value(stream)) {
    opened_tcp_client_sockets->set_value(stream, fdopen(fd, "r"));
  }
  FILE * src = opened_tcp_client_sockets->get_value(stream);
  int res = feof(src);
  dl::leave_critical_section();

  return res != 0;
}

/*
 * now available only blocking/non-blocking option
 * */
bool tcp_stream_set_option (const Stream &stream, int64_t option, int64_t value) {
  int fd = tcp_get_fd(stream);
  if (fd == -1) {
    return false;
  }

  int flags = fcntl(fd, F_GETFL);
  switch (option) {
    case STREAM_SET_BLOCKING_OPTION:
      int res;
      if (value) {
        res = fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
      } else {
        res = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
      }
      return res == 0;
    default:
      return false;
  }
}
} // namespace

void global_init_tcp_lib() {
  static stream_functions tcp_stream_functions;

  tcp_stream_functions.name = string("tcp", 3);
  tcp_stream_functions.fopen = nullptr;
  tcp_stream_functions.fwrite = tcp_fwrite;
  tcp_stream_functions.fseek = nullptr;
  tcp_stream_functions.ftell = nullptr;
  tcp_stream_functions.fread = tcp_fread;
  tcp_stream_functions.fgetc = tcp_fgetc;
  tcp_stream_functions.fgets = tcp_fgets;
  tcp_stream_functions.fpassthru = nullptr;
  tcp_stream_functions.fflush = nullptr;
  tcp_stream_functions.feof = tcp_feof;
  tcp_stream_functions.fclose = tcp_fclose;

  tcp_stream_functions.file_get_contents = nullptr;
  tcp_stream_functions.file_put_contents = nullptr;

  tcp_stream_functions.stream_socket_client = tcp_stream_socket_client;
  tcp_stream_functions.context_set_option = nullptr;
  tcp_stream_functions.stream_set_option = tcp_stream_set_option;
  tcp_stream_functions.get_fd = tcp_get_fd;

  register_stream_functions(&tcp_stream_functions, false);
}

void free_tcp_lib() {
  dl::enter_critical_section();
  if (dl::query_num == opened_tcp_client_sockets_last_query_nam) {
    const array<FILE *> *const_opened_udp_sockets = opened_tcp_client_sockets;
    for (array<FILE *>::const_iterator p = const_opened_udp_sockets->begin(); p != const_opened_udp_sockets->end(); ++p) {
      tcp_fclose(p.get_key());
    }
    opened_tcp_client_sockets_last_query_nam--;
  }
  dl::leave_critical_section();
}
