// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <sys/poll.h>

#include "runtime/critical_section.h"
#include "runtime/datetime/datetime_functions.h"
#include "runtime/net_events.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"
#include "runtime/tcp.h"

namespace {

constexpr int DEFAULT_SOCKET_TIMEOUT = 60;
constexpr addrinfo tcp_hints = {0, AF_UNSPEC, SOCK_STREAM, 0, 0, nullptr, nullptr, nullptr};

int opened_tcp_client_sockets_last_query_num = -1;
std::byte opened_tcp_sockets_storage[sizeof(array<FILE *>)];
array<FILE *> *opened_tcp_client_sockets = reinterpret_cast<array<FILE *> *>(opened_tcp_sockets_storage);

namespace details {
template<typename F>
Optional<std::pair<string, int64_t>> parse_url(const string &url, F &&faulter) {
  string url_to_parse = url;
  if (!url_to_parse.starts_with(string("tcp://"))) {
    faulter(-7, string("\"%s\" doesn't start with tcp"), url_to_parse);
    return {};
  }
  url_to_parse = url_to_parse.substr(strlen("tcp://"), url_to_parse.size() - strlen("tcp://"));
  string::size_type pos = url_to_parse.find(string(":"));
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

template<typename F>
Optional<int64_t> connect_to_address(const string &host, int64_t port, double end_time, F &&faulter) {
  assert(dl::in_critical_section > 0);
  addrinfo *result = nullptr;
  int get_result = getaddrinfo(host.c_str(), string(port).c_str(), &tcp_hints, &result); /*allocation */
  if (get_result != 0 || result == nullptr) {
    faulter(-3, string("Can't resolve host \"%s\""), host);
    return {};
  }
  /* result points to an allocated dynamically linked list of
   * addrinfo structures linked by the ai_next component.
   * There are several reasons for this. We will use the first element */
  addrinfo *rp = result;
  auto finalizer = vk::finally([result]() { freeaddrinfo(result); }); /*free allocation in destructor*/
  int64_t socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
  if (socket_fd == -1) {
    faulter(-3, string("Can't create tcp socket. System call 'socket(...)' got error "), string(errno));
    return {};
  }
  fcntl(socket_fd, F_SETFL, O_NONBLOCK);

  if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != 0) {
    if (errno != EINPROGRESS) {
      faulter(-3, string("Can't connect to tcp socket. System call 'connect(...)' got error "), string(errno));
      close(socket_fd);
      return {};
    }
  }

  pollfd poll_fd{static_cast<int>(socket_fd), POLLOUT, 0};
  double timeout = end_time - microtime_monotonic();
  if (timeout <= 0) {
    faulter(-3, string("Timeout expired %s"), string(""));
    close(socket_fd);
    return {};
  } else if (poll(&poll_fd, 1, timeout_convert_to_ms(timeout)) <= 0) {
    faulter(-3, string("Can't connect to tcp socket. System call 'poll(...)' got error "), string(errno));
    close(socket_fd);
    return {};
  } else {
    int old_flags = fcntl(socket_fd, F_GETFL);
    fcntl(socket_fd, F_SETFL, old_flags & ~O_NONBLOCK);
    return socket_fd;
  }
}

void register_socket(const Stream &stream, int64_t fd) {
  if (dl::query_num != opened_tcp_client_sockets_last_query_num) {
    new (opened_tcp_sockets_storage) array<FILE *>();
    opened_tcp_client_sockets_last_query_num = dl::query_num;
  }
  opened_tcp_client_sockets->set_value(stream, fdopen(fd, "r+"));
}
} // namespace details

Stream tcp_stream_socket_client(const string &url, int64_t &error_number, string &error_description, double timeout, int64_t flags __attribute__((unused)),
                                const mixed &context __attribute__((unused))) {
  dl::CriticalSectionGuard guard;
  if (timeout < 0) {
    timeout = DEFAULT_SOCKET_TIMEOUT;
  }

  double end_time = microtime_monotonic() + timeout;
  auto set_format_error = [&](int64_t error_no, const string &format, string param) {
    error_number = error_no;
    error_description = f$sprintf(format, array<mixed>::create(std::move(param)));
  };

  auto info = details::parse_url(url, set_format_error);
  if (!info.has_value()) {
    php_warning("stream_socket_client() : Unable to connect to %s (%s)", url.c_str(), error_description.c_str());
    return false;
  }

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
  details::register_socket(stream, fd.val());
  return stream;
}

bool tcp_check_correct(const Stream &stream) {
  if (dl::query_num != opened_tcp_client_sockets_last_query_num || !opened_tcp_client_sockets->has_key(stream.to_string())) {
    php_warning("TCP socket \"%s\" is not opened", stream.to_string().c_str());
    return false;
  }

  return true;
}

int tcp_get_fd(const Stream &stream) {
  if (!tcp_check_correct(stream)) {
    return -1;
  }

  FILE *src = opened_tcp_client_sockets->get_value(stream.to_string());
  return fileno(src);
}

Optional<int64_t> tcp_fwrite(const Stream &stream, const string &data) {
  if (!tcp_check_correct(stream)) {
    return {};
  }

  FILE *src = opened_tcp_client_sockets->get_value(stream);
  dl::enter_critical_section();
  int res = fwrite(data.c_str(), sizeof(char), data.size(), src);
  dl::leave_critical_section();

  if (res == -1) {
    if (errno != EAGAIN) {
      php_warning("fwrite() : An error occurred while sending TCP-package");
      return {};
    } else {
      return 0;
    }
  }
  return res;
}

Optional<string> tcp_fread(const Stream &stream, int64_t length) {
  if (!tcp_check_correct(stream)) {
    return {};
  }

  string data(length, false);
  FILE *src = opened_tcp_client_sockets->get_value(stream);
  dl::enter_critical_section();
  int res = fread(data.buffer(), sizeof(char), length, src);
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
  if (!tcp_check_correct(stream)) {
    return {};
  }
  if (length < 0) {
    length = getpagesize();
  }

  string data(length, false);
  FILE *src = opened_tcp_client_sockets->get_value(stream);
  dl::enter_critical_section();
  char *res = fgets(data.buffer(), length, src);
  dl::leave_critical_section();

  if (res == nullptr) {
    return {};
  }
  string::size_type end_pos = data.find(string("\n")) + 1;
  return data.substr(0, end_pos);
}

Optional<string> tcp_fgetc(const Stream &stream) {
  if (!tcp_check_correct(stream)) {
    return {};
  }

  FILE *src = opened_tcp_client_sockets->get_value(stream);
  dl::enter_critical_section();
  int res = fgetc(src);
  dl::leave_critical_section();

  if (res == -1) {
    return {};
  }
  return f$chr(res);
}

bool tcp_fclose(const Stream &stream) {
  if (!tcp_check_correct(stream)) {
    return false;
  }

  FILE *src = opened_tcp_client_sockets->get_value(stream);
  dl::enter_critical_section();
  int res = fclose(src);
  opened_tcp_client_sockets->unset(stream);
  dl::leave_critical_section();

  if (res == -1) {
    php_warning("fclose() : An error occurred while closing TCP-package");
    return false;
  }
  return true;
}

bool tcp_feof(const Stream &stream) {
  if (!tcp_check_correct(stream)) {
    return false;
  }

  FILE *src = opened_tcp_client_sockets->get_value(stream);
  dl::enter_critical_section();
  int res = feof(src);
  dl::leave_critical_section();

  return res != 0;
}

/*
 * now available only blocking/non-blocking option
 * */
bool tcp_stream_set_option(const Stream &stream, int64_t option, int64_t value) {
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

  tcp_stream_functions.name = string("tcp");
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
  if (dl::query_num == opened_tcp_client_sockets_last_query_num) {
    array<FILE *> *const_opened_tcp_sockets = opened_tcp_client_sockets;
    for (array<FILE *>::iterator p = const_opened_tcp_sockets->begin(); p != const_opened_tcp_sockets->end();) {
      Stream key = p.get_key();
      ++p;
      tcp_fclose(key);
    }
    opened_tcp_client_sockets_last_query_num--;
  }
  dl::leave_critical_section();
}
