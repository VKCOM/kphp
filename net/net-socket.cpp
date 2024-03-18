// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-socket.h"

#include <arpa/inet.h>
#include <assert.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/macos-ports.h"
#include "common/options.h"
#include "net/net-socket-options.h"

#define DEFAULT_BACKLOG 8192
int backlog = DEFAULT_BACKLOG;
in_addr settings_addr;

OPTION_PARSER(OPT_NETWORK, "server-bind-to-ipv4", required_argument, "Bind server sockets of all supported protocol types to the specified IPv4 address") {
  char *ip_str = optarg;
  bool success = inet_pton(AF_INET, ip_str, &settings_addr.s_addr);
  return success ? 0 : -1;
}

OPTION_PARSER_SHORT(OPT_NETWORK, "backlog", 'b', required_argument, "sets backlog") {
  set_backlog(optarg);
  return 0;
}

static bool bind_to_my_ipv4;
FLAG_OPTION_PARSER(OPT_NETWORK, "bind-to-my-ipv4", bind_to_my_ipv4, "bind() to PID IP address before connect");

void set_backlog(const char *arg) {
  int new_backlog = atoi(arg);
  if (new_backlog > 0) {
    backlog = new_backlog;
  }
}

const char *unix_socket_path(const char *directory, const char *owner, uint16_t port) {
  assert(directory && owner);

  static struct sockaddr_un path;

  if (snprintf(path.sun_path, sizeof(path.sun_path), "%s/%s/%d", directory, owner, port) >= sizeof(path.sun_path)) {
    kprintf("Too long UNIX socket path: \"%s/%s/%d\": %zu bytes exceeds\n", directory, owner, port, sizeof(path.sun_path));
    return NULL;
  }

  return path.sun_path;
}

int prepare_unix_socket_directory(const char *directory, const char *username, const char *groupname) {
  assert(directory);
  assert(username);
  assert(groupname);

  struct passwd *passwd = getpwnam(username);
  if (!passwd) {
    vkprintf(1, "Cannot getpwnam() for %s: %s\n", username, strerror(errno));
    return -1;
  }

  struct group *group = getgrnam(groupname);
  if (!group) {
    vkprintf(1, "Cannot getgrnam() for %s: %s\n", groupname, strerror(errno));
    return -1;
  }

  int dirfd = open(directory, O_DIRECTORY);
  if (dirfd == -1) {
    if (errno == ENOENT) {
      vkprintf(4, "Trying to create UNIX socket directory: \"%s\"\n", directory);
      const mode_t dirmode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
      if (mkdir(directory, dirmode) == -1 && errno != EEXIST) {
        vkprintf(1, "Cannot mkdir() UNIX socket directory: \"%s\": %s\n", directory, strerror(errno));
        return -1;
      }
    }

    dirfd = open(directory, O_DIRECTORY);
  }

  if (dirfd == -1) {
    vkprintf(1, "Cannot open() UNIX socket directory: \"%s\": %s\n", directory, strerror(errno));
    return -1;
  }

  int groupdirfd = openat(dirfd, group->gr_name, O_DIRECTORY);
  if (groupdirfd == -1) {
    if (errno == ENOENT) {
      if (mkdirat(dirfd, group->gr_name, S_IRUSR) == -1 && errno != EEXIST) {
        vkprintf(1, "Cannot mkdirat() UNIX socket group directory: \"%s/%s\": %s\n", directory, groupname, strerror(errno));
        close(dirfd);
        return -1;
      }

      groupdirfd = openat(dirfd, group->gr_name, O_DIRECTORY);
    }

    if (groupdirfd == -1) {
      vkprintf(1, "Cannot openat() UNIX socket group directory: \"%s/%s\": %s\n", directory, groupname, strerror(errno));
      close(dirfd);
      return -1;
    }
  }

  struct stat groupdirst;
  if (fstat(groupdirfd, &groupdirst) == -1) {
    vkprintf(1, "Cannot fstatat() UNIX socket group directory: \"%s/%s\": %s\n", directory, groupname, strerror(errno));
    close(groupdirfd);
    close(dirfd);
    return -1;
  }

  if (groupdirst.st_uid != passwd->pw_uid || groupdirst.st_gid != group->gr_gid) {
    if (fchown(groupdirfd, passwd->pw_uid, group->gr_gid)) {
      vkprintf(1, "Cannot fchown() UNIX socket group directory: \"%s/%s\": %s\n", directory, groupname, strerror(errno));
      close(groupdirfd);
      close(dirfd);
      return -1;
    }
  }

  const mode_t groupdirmode = S_IRWXU | S_IRGRP | S_IXGRP;
  if ((groupdirst.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != groupdirmode) {
    if (fchmod(groupdirfd, groupdirmode) == -1) {
      vkprintf(1, "Cannot fchmod() UNIX socket owner directory: \"%s/%s\": %s\n", directory, username, strerror(errno));
      close(groupdirfd);
      close(dirfd);
      return -1;
    }
  }

  close(groupdirfd);
  close(dirfd);

  return 0;
}

int prepare_unix_socket(const char *path, const char *username, const char *groupname) {
  assert(path);
  assert(username);
  assert(groupname);

  const struct passwd *passwd = getpwnam(username);
  if (!passwd) {
    vkprintf(1, "Cannot getpwnam() for %s: %s\n", username, strerror(errno));
    return -1;
  }

  const struct group *group = getgrnam(groupname);
  if (!group) {
    vkprintf(1, "Cannot getgrnam() for %s: %s\n", groupname, strerror(errno));
    return -1;
  }

  if (chown(path, passwd->pw_uid, group->gr_gid)) {
    vkprintf(1, "Cannot chown() \"%s\" to %s:%s: %s\n", path, username, groupname, strerror(errno));
    return -1;
  }

  const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
  if (chmod(path, mode)) {
    vkprintf(1, "Cannot chmod() \"%s\" to %u: %s\n", path, mode, strerror(errno));
    return -1;
  }

  return 0;
}

bool set_fd_nonblocking(int fd) {
  const int flags = fcntl(fd, F_GETFL, 0);
  return flags != -1 && (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

static int new_socket(int mode, int nonblock) {
  int sfd;

  const int domain = mode & SM_IPV6 ? AF_INET6 : (mode & SM_UNIX ? AF_UNIX : AF_INET);
  const int type = SOCK_CLOEXEC | (mode & SM_DGRAM ? SOCK_DGRAM : SOCK_STREAM);

  if ((sfd = socket(domain, type, 0)) == -1) {
    perror("socket()");
    return -1;
  }

  if (mode & SM_IPV6) {
    const bool result = mode & SM_IPV6_ONLY ? socket_enable_ip_v6_only(sfd) : socket_disable_ip_v6_only(sfd);
    if (!result) {
      perror("setting IPV6_V6ONLY");
      close(sfd);
      return -1;
    }
  }

  if (!nonblock) {
    return sfd;
  }

  if (!set_fd_nonblocking(sfd)) {
    kprintf("Cannot set O_NONBLOCK on fd: %d: %m\n", sfd);
    close(sfd);
    return -1;
  }

  return sfd;
}

int server_socket(int port, struct in_addr in_addr, int backlog, int mode) {
  const int sfd = new_socket(mode, 1);

  if (sfd == -1) {
    return -1;
  }

  if (mode & SM_DGRAM) {
    socket_maximize_sndbuf(sfd, 0);
    socket_maximize_rcvbuf(sfd, 0);
    socket_enable_ip_receive_errors(sfd);
  } else {
    socket_enable_reuseaddr(sfd);
    socket_maximize_sndbuf(sfd, 0);
    socket_maximize_rcvbuf(sfd, 0);
    socket_enable_keepalive(sfd);
    socket_disable_linger(sfd);
    socket_enable_tcp_nodelay(sfd);
  }

  if (mode & SM_REUSE) {
    socket_enable_reuseaddr(sfd);
  }

  if (mode & SM_REUSEPORT) {
    if (!socket_enable_reuseport(sfd)) {
      close(sfd);
      return -1;
    }
  }

  if (!(mode & SM_IPV6)) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = in_addr;
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      kprintf("bind(%s:%d): %s\n", inet_ntoa(in_addr), port, strerror(errno));
      close(sfd);
      return -1;
    }
  } else {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      kprintf("bind(%s:%d): %s\n", inet_ntoa(in_addr), port, strerror(errno));
      close(sfd);
      return -1;
    }
  }

  if (mode & SM_DGRAM) {
    if (!socket_enable_ip_receive_packet_info(sfd)) {
      kprintf("setsockopt for %d failed: %m\n", sfd);
    }

    if ((mode & SM_IPV6) && !socket_enable_ip_v6_receive_packet_info(sfd)) {
      kprintf("setsockopt for %d failed: %m\n", sfd);
    }
  }

  if (!(mode & SM_DGRAM) && listen(sfd, backlog) == -1) {
    perror("listen()");
    close(sfd);
    return -1;
  }
  return sfd;
}

int server_socket_unix(const struct sockaddr_un *addr, int backlog, int mode) {
  assert(mode & SM_UNIX);

  const int fd = new_socket(mode, 1);

  if (fd != -1) {
    socket_maximize_rcvbuf(fd, 0);
    socket_maximize_sndbuf(fd, 0);

    vkprintf(4, "Unlinking UNIX socket path: \"%s\"\n", addr->sun_path);
    unlink(addr->sun_path);

    if (bind(fd, (struct sockaddr *)addr, sizeof(*addr)) == -1) {
      kprintf("bind(%s): %s\n", addr->sun_path, strerror(errno));
      close(fd);
      return -1;
    }

    if (!(mode & SM_DGRAM) && listen(fd, backlog) == -1) {
      perror("listen()");
      close(fd);
      return -1;
    }
  }

  return fd;
}

int client_socket(const struct sockaddr_in *addr, int mode) {
  int sfd;

  if (mode & SM_IPV6) {
    return -1;
  }

  if ((sfd = new_socket(mode, 1)) == -1) {
    return -1;
  }

  if (mode & SM_DGRAM) {
    socket_maximize_sndbuf(sfd, 0);
    socket_maximize_rcvbuf(sfd, 0);
    socket_enable_ip_receive_errors(sfd);
  } else {
    socket_enable_reuseaddr(sfd);
    socket_maximize_sndbuf(sfd, 0);
    socket_maximize_rcvbuf(sfd, 0);
    socket_enable_keepalive(sfd);
    socket_enable_tcp_nodelay(sfd);
  }

  if (bind_to_my_ipv4) {
    struct sockaddr_in bind_to;
    memset(&bind_to, 0, sizeof(bind_to));
    bind_to.sin_family = AF_INET;
    bind_to.sin_addr.s_addr = htonl(PID.ip);

    if (bind(sfd, reinterpret_cast<const struct sockaddr *>(&bind_to), sizeof(bind_to)) == -1) {
      kprintf("Cannot bind() to my IPv4 address: %s\n", strerror(errno));
      close(sfd);
      return -1;
    }
  }

  if (connect(sfd, (struct sockaddr *)addr, sizeof(*addr)) == -1 && errno != EINPROGRESS) {
    perror("connect()");
    close(sfd);
    return -1;
  }

  return sfd;
}

int client_socket_ipv6(const struct sockaddr_in6 *addr, int mode) {
  int sfd;

  if (!(mode & SM_IPV6)) {
    return -1;
  }

  if ((sfd = new_socket(mode, 1)) == -1) {
    return -1;
  }

  if (mode & SM_DGRAM) {
    socket_maximize_sndbuf(sfd, 0);
    socket_maximize_rcvbuf(sfd, 0);
  } else {
    socket_enable_reuseaddr(sfd);
    socket_maximize_sndbuf(sfd, 0);
    socket_maximize_rcvbuf(sfd, 0);
    socket_enable_keepalive(sfd);
    socket_enable_tcp_nodelay(sfd);
  }

  if (connect(sfd, (struct sockaddr *)addr, sizeof(*addr)) == -1 && errno != EINPROGRESS) {
    perror("connect()");
    close(sfd);
    return -1;
  }

  return sfd;
}

int client_socket_unix(const struct sockaddr_un *addr, int mode) {
  assert(mode & SM_UNIX);

  const int fd = new_socket(mode, 1);

  if (fd != -1) {
    socket_maximize_sndbuf(fd, 0);
    socket_maximize_rcvbuf(fd, 0);
    if (connect(fd, (struct sockaddr *)addr, sizeof(*addr)) == -1 && errno != EINPROGRESS) {
      kprintf("Cannot connect() to \"%s\": %s\n", addr->sun_path, strerror(errno));
      close(fd);
      return -1;
    }
  }

  return fd;
}
