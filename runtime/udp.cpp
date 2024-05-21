// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/udp.h"

#include <cerrno>
#include <poll.h>
#include <sys/socket.h>

#include "common/resolver.h"

#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/datetime/datetime_functions.h"
#include "runtime/net_events.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"
#include "runtime/url.h"

int DEFAULT_SOCKET_TIMEOUT = 60;

static char opened_udp_sockets_storage[sizeof(array<int>)];
static array<int> *opened_udp_sockets = reinterpret_cast <array<int> *> (opened_udp_sockets_storage);
static long long opened_udp_sockets_last_query_num = -1;

static Stream udp_stream_socket_client(const string &url, int64_t &error_number, string &error_description, double timeout,
                                       int64_t flags __attribute__((unused)), const mixed &options __attribute__((unused))) {
  auto quit = [&](int socket_fd, const Optional<string> & extra_info = {}) {
    if (extra_info.has_value()) {
      php_warning("%s. %s", error_description.c_str(), extra_info.val().c_str());
    } else {
      php_warning("%s", error_description.c_str());
    }
    if (socket_fd != -1) {
      close(socket_fd);
    }
    return false;
  };
  auto set_error = [&](int64_t error_no, const char * error) {
    error_number = error_no;
    error_description = CONST_STRING(error);
  };
  auto set_format_error = [&](int64_t error_no, const char * format, auto ... params) {
    error_number = error_no;
    error_description = f$sprintf(CONST_STRING(format), array<mixed>::create(params...));
  };

  if (timeout < 0) {
    timeout = DEFAULT_SOCKET_TIMEOUT;
  }
  double end_time = microtime_monotonic() + timeout;
  int sock_fd = -1;
  string url_to_parse = url;

  if (url_to_parse.size() < 6 || url_to_parse.substr(0, 6) != string("udp://", 6)) {
    set_format_error(-7, "\"%s\" doesn't start with UDP", url);
    return quit(sock_fd);
  }
  url_to_parse = url_to_parse.substr(6, url_to_parse.size() - 6);
  string::size_type pos = url_to_parse.find(string(":", 1));
  if (pos == string::npos) {
    set_format_error(-7, "\"%s\" has to be of format 'udp://<host>:<port>'", url);
    return quit(sock_fd);
  }
  string host = url_to_parse.substr(0, pos);
  int64_t port = f$intval(url_to_parse.substr(pos + 1, url_to_parse.size() - (pos + 1)));

  if (host.empty()) {
    set_format_error(-1, "Wrong host specified in url \"%s\"", url);
    return quit(sock_fd);
  }

  if (port <= 0 || port >= 65536) {
    set_format_error(-2, "Wrong port specified in url \"%s\"", url);
    return quit(sock_fd);
  }

  // gethostbyname often uses heap => it must be under critical section, otherwise we will get UB on timeout in the middle of it
  struct hostent *h = dl::critical_section_call(kdb_gethostbyname, host.c_str());
  if (!h || !h->h_addr_list || !h->h_addr_list[0]) {
    set_format_error(-3, "Can't resolve host \"%s\"", host);
    return quit(sock_fd);
  }

  struct sockaddr_storage addr;
  addr.ss_family = static_cast<sa_family_t>(h->h_addrtype);
  int addrlen;
  switch (h->h_addrtype) {
    case AF_INET:
      php_assert (h->h_length == 4);
      (reinterpret_cast <struct sockaddr_in *> (&addr))->sin_port = htons(static_cast<uint16_t>(port));
      (reinterpret_cast <struct sockaddr_in *> (&addr))->sin_addr = *(struct in_addr *)h->h_addr;
      addrlen = sizeof(struct sockaddr_in);
      break;
    case AF_INET6:
      php_assert (h->h_length == 16);
      (reinterpret_cast <struct sockaddr_in6 *> (&addr))->sin6_port = htons(static_cast<uint16_t>(port));
      memcpy(&(reinterpret_cast <struct sockaddr_in6 *> (&addr))->sin6_addr, h->h_addr, h->h_length);
      addrlen = sizeof(struct sockaddr_in6);
      break;
    default:
      //there is no other known address types
      php_assert (0);
  }
  dl::enter_critical_section();
  sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_fd == -1) {
    dl::leave_critical_section();
    set_error(-4, "Can't create udp socket");
    return quit(sock_fd, string("System call 'socket(...)' got error '").append(strerror(errno)).append("'"));
  }
  if (connect(sock_fd, reinterpret_cast <const sockaddr *> (&addr), addrlen) == -1) {
    if (errno != EINPROGRESS) {
      dl::leave_critical_section();
      set_error(-5, "Can't connect to udp socket");
      return quit(sock_fd, string("System call 'connect(...)' got error '").append(strerror(errno)).append("'"));
    }

    pollfd poll_fds;
    poll_fds.fd = sock_fd;
    poll_fds.events = POLLIN | POLLERR | POLLHUP | POLLOUT | POLLPRI;

    double left_time = end_time - microtime_monotonic();
    if (left_time <= 0 || poll(&poll_fds, 1, timeout_convert_to_ms(left_time)) <= 0) {
      dl::leave_critical_section();
      string extra_info;
      if (left_time <= 0) {
        extra_info = string("Timeout expired");
      } else {
        extra_info = string("System call 'poll(...)' got error '").append(strerror(errno)).append("'");
      }
      set_error(-6, "Can't connect to udp socket");
      return quit(sock_fd, extra_info);
    }
  }

  if (dl::query_num != opened_udp_sockets_last_query_num) {
    new(opened_udp_sockets_storage) array<int>();
    opened_udp_sockets_last_query_num = dl::query_num;
  }
  string stream_key = url;
  if (opened_udp_sockets->has_key(stream_key)) {
    int try_count;
    for (try_count = 0; try_count < 3; try_count++) {
      stream_key = url;
      stream_key.append("#", 1);
      stream_key.append(string(rand()));

      if (!opened_udp_sockets->has_key(stream_key)) {
        break;
      }
    }

    if (try_count == 3) {
      dl::leave_critical_section();
      set_error(-18, "Can't generate stream_name in 3 tries. Something is definitely wrong.");
      return quit(sock_fd);
    }
  }
  opened_udp_sockets->set_value(stream_key, sock_fd);
  dl::leave_critical_section();
  return stream_key;
}

static int udp_get_fd(const Stream &stream) {
  string stream_key = stream.to_string();
  if (dl::query_num != opened_udp_sockets_last_query_num || !opened_udp_sockets->has_key(stream_key)) {
    php_warning("UDP socket \"%s\" is not opened", stream_key.c_str());
    return -1;
  }
  return opened_udp_sockets->get_value(stream_key);
}

static Optional<int64_t> udp_fwrite(const Stream &stream, const string &data) {
  int sock_fd = udp_get_fd(stream);
  if (sock_fd == -1) {
    return false;
  }
  const void *data_ptr = static_cast <const void *> (data.c_str());
  size_t data_len = data.size();
  if (data_len == 0) {
    return 0;
  }
  dl::enter_critical_section(); // OK
  ssize_t res = send(sock_fd, data_ptr, data_len, 0);
  dl::leave_critical_section();
  if (res == -1) {
    php_warning("An error occurred while sending UPD-package");
    return false;
  }
  return res;
}

static bool udp_fclose(const Stream &stream) {
  string stream_key = stream.to_string();

  if (dl::query_num != opened_udp_sockets_last_query_num || !opened_udp_sockets->has_key(stream_key)) {
    return false;
  }

  dl::enter_critical_section();
  int result = close(opened_udp_sockets->get_value(stream_key));
  opened_udp_sockets->unset(stream_key);
  dl::leave_critical_section();
  return result == 0;
}

void global_init_udp_lib() {
  static stream_functions udp_stream_functions;

  udp_stream_functions.name = string("udp", 3);
  udp_stream_functions.fopen = nullptr;
  udp_stream_functions.fwrite = udp_fwrite;
  udp_stream_functions.fseek = nullptr;
  udp_stream_functions.ftell = nullptr;
  udp_stream_functions.fread = nullptr;
  udp_stream_functions.fgetc = nullptr;
  udp_stream_functions.fgets = nullptr;
  udp_stream_functions.fpassthru = nullptr;
  udp_stream_functions.fflush = nullptr;
  udp_stream_functions.feof = nullptr;
  udp_stream_functions.fclose = udp_fclose;

  udp_stream_functions.file_get_contents = nullptr;
  udp_stream_functions.file_put_contents = nullptr;

  udp_stream_functions.stream_socket_client = udp_stream_socket_client;
  udp_stream_functions.context_set_option = nullptr;
  udp_stream_functions.stream_set_option = nullptr;
  udp_stream_functions.get_fd = udp_get_fd;

  register_stream_functions(&udp_stream_functions, false);
}

void free_udp_lib() {
  dl::enter_critical_section();//OK
  if (dl::query_num == opened_udp_sockets_last_query_num) {
    const array<int> *const_opened_udp_sockets = opened_udp_sockets;
    for (array<int>::const_iterator p = const_opened_udp_sockets->begin(); p != const_opened_udp_sockets->end(); ++p) {
      close(p.get_value());
    }
    opened_udp_sockets_last_query_num--;
  }
  dl::leave_critical_section();
}
