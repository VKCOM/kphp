// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include "curl/curl.h"
#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"

#if defined(__linux__)
// Linker wrapper used only by this test executable. All other names use libc.
static uint64_t resolver_wait_ns = 0;
extern "C" int __real_getaddrinfo(const char*, const char*, const addrinfo*, addrinfo**);
extern "C" int __wrap_getaddrinfo(const char* host, const char* service, const addrinfo* hints, addrinfo** result) {
  if (host && std::strcmp(host, "builtin-timer.invalid") == 0) {
    const auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    resolver_wait_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    return EAI_NONAME;
  }
  return __real_getaddrinfo(host, service, hints, result);
}
#endif

namespace {

class KeepAliveServer {
public:
  KeepAliveServer() {
    listener_ = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t size = sizeof(address);
    if (listener_ < 0 || bind(listener_, reinterpret_cast<sockaddr*>(&address), size) != 0 ||
        getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &size) != 0 || listen(listener_, 1) != 0) {
      if (listener_ >= 0) {
        close(listener_);
      }
      throw std::runtime_error("could not start loopback HTTP test server");
    }
    url = "http://127.0.0.1:" + std::to_string(ntohs(address.sin_port)) + "/";
    worker_ = std::thread([this] {
      const int client = accept(listener_, nullptr, nullptr);
      client_ = client;
      if (client < 0) {
        return;
      }
      const timeval timeout{2, 0};
      setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      for (int request = 0; request < 2 && !stopping_; ++request) {
        std::string header;
        char byte;
        while (header.find("\r\n\r\n") == std::string::npos) {
          if (recv(client, &byte, 1, 0) != 1) {
            return;
          }
          header.push_back(byte);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        const char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: keep-alive\r\n\r\nok";
#ifdef MSG_NOSIGNAL
        constexpr int send_flags = MSG_NOSIGNAL;
#else
        constexpr int send_flags = 0;
#endif
        if (send(client, response, sizeof(response) - 1, send_flags) != static_cast<ssize_t>(sizeof(response) - 1)) {
          return;
        }
      }
    });
  }

  ~KeepAliveServer() {
    stopping_ = true;
    shutdown(listener_, SHUT_RDWR);
    if (client_ >= 0) {
      shutdown(client_, SHUT_RDWR);
    }
    worker_.join();
    if (client_ >= 0) {
      close(client_);
    }
    close(listener_);
  }

  std::string url;

private:
  int listener_;
  std::atomic<int> client_{-1};
  std::atomic<bool> stopping_{false};
  std::thread worker_;
};

size_t receive_body(char*, size_t size, size_t count, void*) {
  // This callback's work must remain measured even though libcurl invokes it.
  std::this_thread::sleep_for(std::chrono::milliseconds{5});
  return size * count;
}

TEST(curl_active_time_test, excludes_network_wait_and_reuses_connection_after_reset) {
  ASSERT_EQ(curl_global_init(CURL_GLOBAL_DEFAULT), CURLE_OK);
  KeepAliveServer server;
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> easy{curl_easy_init(), curl_easy_cleanup};
  ASSERT_NE(easy, nullptr);
  uint64_t total = 0;
  uint64_t method = 0;
  const auto started_at = std::chrono::steady_clock::now();
  for (int attempt = 0; attempt < 2; ++attempt) {
    curl_easy_reset(easy.get());
    ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_URL, server.url.c_str()), CURLE_OK);
    ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_NOPROXY, "*"), CURLE_OK);
    ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_TIMEOUT_MS, 2000L), CURLE_OK);
    ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, receive_body), CURLE_OK);
    {
      BuiltinTimeGuard timer{total, method};
      BuiltinTimeNetworkScope network_scope{timer};
      ASSERT_EQ(std::invoke(curl_easy_perform, easy.get()), CURLE_OK);
    }
    long connections = -1;
    ASSERT_EQ(curl_easy_getinfo(easy.get(), CURLINFO_NUM_CONNECTS, &connections), CURLE_OK);
    EXPECT_EQ(connections, attempt == 0 ? 1 : 0);
  }
  const auto wall_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started_at).count();
  EXPECT_EQ(total, method);
  EXPECT_GE(total, 10'000'000);
  ASSERT_GE(wall_ns, total);
  EXPECT_GE(wall_ns - total, 100'000'000);
}

TEST(curl_active_time_test, rejects_an_easy_handle_already_owned_by_another_multi) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> easy{curl_easy_init(), curl_easy_cleanup};
  std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> multi{curl_multi_init(), curl_multi_cleanup};
  ASSERT_EQ(curl_multi_add_handle(multi.get(), easy.get()), CURLM_OK);
  uint64_t total = 0;
  uint64_t method = 0;
  BuiltinTimeGuard timer{total, method};
  BuiltinTimeNetworkScope network_scope{timer};
  EXPECT_EQ(std::invoke(curl_easy_perform, easy.get()), CURLE_FAILED_INIT);
  EXPECT_EQ(curl_multi_remove_handle(multi.get(), easy.get()), CURLM_OK);
}

TEST(curl_active_time_test, error_and_timeout_paths_detach_the_easy_handle) {
  ASSERT_EQ(curl_global_init(CURL_GLOBAL_DEFAULT), CURLE_OK);
  KeepAliveServer server;
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> easy{curl_easy_init(), curl_easy_cleanup};
  ASSERT_NE(easy, nullptr);
  uint64_t total = 0;
  uint64_t method = 0;
  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_NOPROXY, "*"), CURLE_OK);
  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_URL, "nosuch://127.0.0.1/"), CURLE_OK);
  {
    BuiltinTimeGuard timer{total, method};
    BuiltinTimeNetworkScope network_scope{timer};
    EXPECT_EQ(std::invoke(curl_easy_perform, easy.get()), CURLE_UNSUPPORTED_PROTOCOL);
  }
  const auto error_time = total;
  ASSERT_GT(error_time, 0);

  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_URL, server.url.c_str()), CURLE_OK);
  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_NOPROXY, "*"), CURLE_OK);
  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_TIMEOUT_MS, 10L), CURLE_OK);
  {
    BuiltinTimeGuard timer{total, method};
    BuiltinTimeNetworkScope network_scope{timer};
    EXPECT_EQ(std::invoke(curl_easy_perform, easy.get()), CURLE_OPERATION_TIMEDOUT);
  }
  EXPECT_GT(total, error_time);
  EXPECT_EQ(total, method);
  std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> other_multi{curl_multi_init(), curl_multi_cleanup};
  ASSERT_NE(other_multi, nullptr);
  EXPECT_EQ(curl_multi_add_handle(other_multi.get(), easy.get()), CURLM_OK);
  EXPECT_EQ(curl_multi_remove_handle(other_multi.get(), easy.get()), CURLM_OK);
}

#if defined(__linux__)
TEST(curl_active_time_test, excludes_hidden_blocking_resolver_without_changing_error) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> easy{curl_easy_init(), curl_easy_cleanup};
  ASSERT_NE(easy, nullptr);
  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_URL, "http://builtin-timer.invalid/"), CURLE_OK);
  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_NOPROXY, "*"), CURLE_OK);
  ASSERT_EQ(curl_easy_setopt(easy.get(), CURLOPT_NOSIGNAL, 1L), CURLE_OK);
  uint64_t total{}, method{};
  resolver_wait_ns = 0;
  const auto started = std::chrono::steady_clock::now();
  {
    BuiltinTimeGuard timer{total, method};
    BuiltinTimeNetworkScope network_scope{timer};
    EXPECT_EQ(curl_easy_perform(easy.get()), CURLE_COULDNT_RESOLVE_HOST);
  }
  const auto wall_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count();
  ASSERT_GE(resolver_wait_ns, 100'000'000);
  ASSERT_GE(wall_ns, total);
  EXPECT_GE(wall_ns - total, resolver_wait_ns);
}
#endif

} // namespace
