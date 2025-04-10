// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#if defined(__APPLE__)
  #include <cassert>
  #include <cerrno>
  #include <fcntl.h>
  #include <libgen.h>
  #include <libkern/OSByteOrder.h>
  #include <malloc/malloc.h>
  #include <sys/socket.h>
  #include <sys/syscall.h>
  #include <unistd.h>

  #define htobe64(x) OSSwapHostToBigInt64(x)
  #define htole64(x) OSSwapHostToLittleInt64(x)
  #define be64toh(x) OSSwapBigToHostInt64(x)
  #define le64toh(x) OSSwapLittleToHostInt64(x)

  #define SIGPOLL SIGIO

  #define AF_FILE AF_LOCAL

  #define POLLRDHUP 0x2000

  #define SO_PEERCRED 0x1017

  #define SCM_CREDENTIALS 0x02

  #define st_mtim st_mtimespec

  #define FUTEX_LOCK_PI 6
  #define FUTEX_UNLOCK_PI 7
  #define FUTEX_TRYLOCK_PI 8
  #define FUTEX_WAITERS 0x80000000

extern char** environ;

struct ucred {
  pid_t pid;
  uid_t uid;
  gid_t gid;
};

inline const char* strchrnul(const char* s, char c) noexcept {
  while (*s && *s != c) {
    ++s;
  }
  return s;
}

inline const void* memrchr(const void* s, int c, size_t n) noexcept {
  if (n != 0) {
    const unsigned char* cp = static_cast<const unsigned char*>(s) + n;
    do {
      if (*(--cp) == static_cast<unsigned char>(c)) {
        return cp;
      }
    } while (--n != 0);
  }
  return nullptr;
}

inline char* __xpg_basename(char* path) noexcept {
  return basename(path);
}

inline int pipe2(int pipefd[2], int flags) noexcept {
  const int res = pipe(pipefd);
  if (res != -1) {
    if (flags & O_CLOEXEC) {
      fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
      fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
    }
    if (flags & O_NONBLOCK) {
      fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
      fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
    }
  }
  return res;
}

inline ssize_t sendfile(int out_fd, int in_fd, off_t* offset, size_t) noexcept {
  assert(!offset && "offset is not supported");
  off_t len = 0;
  ssize_t s = sendfile(in_fd, out_fd, 0, &len, nullptr, 0);
  if (!s) {
    s = static_cast<ssize_t>(len);
  }
  return s;
}

  #define SOCK_CLOEXEC FD_CLOEXEC

inline int accept4(int sockfd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
  assert((!flags || flags == SOCK_CLOEXEC) && "only SOCK_CLOEXEC is supported");

  int fd = accept(sockfd, addr, addrlen);
  if (fd > 0 && flags == SOCK_CLOEXEC) {
    fcntl(fd, F_SETFD, SOCK_CLOEXEC);
  }
  return fd;
}

  #define PR_SET_PDEATHSIG 1
  #define PR_SET_DUMPABLE 4

inline int prctl(int, unsigned long) noexcept {
  errno = EINVAL;
  return -1;
}

  #define PTHREAD_MUTEX_ROBUST_NP 2

inline int pthread_mutexattr_setrobust(const pthread_mutexattr_t*, int) noexcept {
  return 0;
}

inline int pthread_mutex_consistent(pthread_mutex_t*) noexcept {
  return 0;
}

struct mallinfo_port {
  int hblkhd{0};
  int uordblks{0};
  int fordblks{0};
};

inline mallinfo_port mallinfo() noexcept {
  return mallinfo_port{};
}

inline size_t malloc_usable_size(void* ptr) noexcept {
  return malloc_size(ptr);
}

#else

  #include <linux/futex.h>
  #include <malloc.h>
  #include <sys/prctl.h>
  #include <sys/sendfile.h>

#endif
