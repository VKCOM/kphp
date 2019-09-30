#include "runtime/udp.h"

#include <cerrno>
#include <poll.h>
#include <sys/socket.h>

#include "common/resolver.h"

#include "runtime/critical_section.h"
#include "runtime/datetime.h"
#include "runtime/net_events.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"
#include "runtime/url.h"

int DEFAULT_SOCKET_TIMEOUT = 60;

static char opened_udp_sockets_storage[sizeof(array<int>)];
static array<int> *opened_udp_sockets = reinterpret_cast <array<int> *> (opened_udp_sockets_storage);
static long long opened_udp_sockets_last_query_num = -1;

static Stream udp_stream_socket_client(const string &url, int &error_number, string &error_description, double timeout, int flags __attribute__((unused)), const var &options __attribute__((unused))) {
#define RETURN                                          \
  php_warning ("%s", error_description.c_str());        \
  if (sock_fd != -1) {                                  \
    close(sock_fd);                                     \
  }                                                     \
  return false

#define RETURN_ERROR(error_no, error)                   \
  error_number = error_no;                              \
  error_description = CONST_STRING(error);              \
  RETURN

#define RETURN_ERROR_FORMAT(error_no, format, ...)                   \
  error_number = error_no;                                           \
  error_description = f$sprintf (                                    \
    CONST_STRING(format), array<var>::create(__VA_ARGS__));          \
  RETURN
  if (timeout < 0) {
    timeout = DEFAULT_SOCKET_TIMEOUT;
  }
  double end_time = microtime_monotonic() + timeout;
  int sock_fd = -1;
  string url_to_parse = url;

  if (url_to_parse.size() < 6 || url_to_parse.substr(0, 6) != string("udp://", 6)) {
    RETURN_ERROR_FORMAT(-7, "\"%s\" doesn't start with UDP", url);
  }
  url_to_parse = url_to_parse.substr(6, url_to_parse.size() - 6);
  string::size_type pos = url_to_parse.find(string(":", 1));
  if (pos == string::npos) {
    RETURN_ERROR_FORMAT(-7, "\"%s\" has to be of format 'udp://<host>:<port>'", url);
  }
  string host = url_to_parse.substr(0, pos);
  int port = f$intval(url_to_parse.substr(pos + 1, url_to_parse.size() - (pos + 1)));

  if (host.empty()) {
    RETURN_ERROR_FORMAT(-1, "Wrong host specified in url \"%s\"", url);
  }

  if (port <= 0 || port >= 65536) {
    RETURN_ERROR_FORMAT(-2, "Wrong port specified in url \"%s\"", url);
  }

  struct hostent *h;
  if (!(h = kdb_gethostbyname(host.c_str())) || !h->h_addr_list || !h->h_addr_list[0]) {
    RETURN_ERROR_FORMAT(-3, "Can't resolve host \"%s\"", host);
  }

  struct sockaddr_storage addr;
  addr.ss_family = h->h_addrtype;
  int addrlen;
  switch (h->h_addrtype) {
    case AF_INET:
      php_assert (h->h_length == 4);
      (reinterpret_cast <struct sockaddr_in *> (&addr))->sin_port = htons(port);
      (reinterpret_cast <struct sockaddr_in *> (&addr))->sin_addr = *(struct in_addr *)h->h_addr;
      addrlen = sizeof(struct sockaddr_in);
      break;
    case AF_INET6:
      php_assert (h->h_length == 16);
      (reinterpret_cast <struct sockaddr_in6 *> (&addr))->sin6_port = htons(port);
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
    RETURN_ERROR(-4, "Can't create udp socket");
  }
  if (connect(sock_fd, reinterpret_cast <const sockaddr *> (&addr), addrlen) == -1) {
    if (errno != EINPROGRESS) {
      dl::leave_critical_section();
      RETURN_ERROR(-5, "Can't connect to udp socket");
    }

    pollfd poll_fds;
    poll_fds.fd = sock_fd;
    poll_fds.events = POLLIN | POLLERR | POLLHUP | POLLOUT | POLLPRI;

    double left_time = end_time - microtime_monotonic();
    if (left_time <= 0 || poll(&poll_fds, 1, timeout_convert_to_ms(left_time)) <= 0) {
      dl::leave_critical_section();
      RETURN_ERROR(-6, "Can't connect to udp socket");
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
      RETURN_ERROR(-18, "Can't generate stream_name in 3 tries. Something is definitely wrong.");
    }
  }
  opened_udp_sockets->set_value(stream_key, sock_fd);
  dl::leave_critical_section();
  return stream_key;
#undef RETURN
#undef RETURN_ERROR
#undef RETURN_ERROR_FORMAT
}

static int udp_get_fd(const Stream &stream) {
  string stream_key = stream.to_string();
  if (dl::query_num != opened_udp_sockets_last_query_num || !opened_udp_sockets->has_key(stream_key)) {
    php_warning("UDP socket \"%s\" is not opened", stream_key.c_str());
    return -1;
  }
  return opened_udp_sockets->get_value(stream_key);
}

static Optional<int> udp_fwrite(const Stream &stream, const string &data) {
  int sock_fd = udp_get_fd(stream);
  if (sock_fd == -1) {
    return false;
  }
  const void *data_ptr = static_cast <const void *> (data.c_str());
  size_t data_len = static_cast <size_t> (data.size());
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
  return (int)res;
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
