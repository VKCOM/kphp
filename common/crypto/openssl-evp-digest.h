#pragma once

#include <cassert>
#include <cstdint>
#include <openssl/evp.h>

#include "common/smart_ptrs/unique_ptr_with_delete_function.h"
#include "common/wrappers/openssl.h"
#include "common/wrappers/span.h"

namespace vk {

template<const EVP_MD *(*digest_fetcher)()>
class digest {
public:
  digest() noexcept {
    hash_init();
  }
  ~digest() noexcept = default;

  digest(const digest &) = delete;
  digest(digest &&other) noexcept { std::swap(this->ctx_, other.ctx_); }

  digest &operator=(const digest &other) = delete;
  digest &operator=(digest &&other) noexcept {
    std::swap(this->ctx_, other.ctx_);
    return *this;
  }

  void hash_init() noexcept {
    // nullptr algorithm means previous ctx re-initialization
    const EVP_MD *algorithm = nullptr;
    if (ctx_ == nullptr) {
      ctx_.reset(EVP_MD_CTX_new());
      assert(ctx_ != nullptr);
      algorithm = digest_fetcher();
    }
    auto res = EVP_DigestInit_ex(ctx_.get(), algorithm, nullptr);
    assert(res == 1);
  }

  void hash_update(vk::span<const uint8_t> data) noexcept {
    assert(ctx_ != nullptr);
    auto res = EVP_DigestUpdate(ctx_.get(), data.data(), data.size());
    assert(res == 1);
  }

  void hash_final(vk::span<uint8_t> out, bool reuse = false) noexcept {
    assert(ctx_ != nullptr);
    const auto size = result_size();
    assert(size <= out.size());
    unsigned int len = 0;
    auto res = EVP_DigestFinal_ex(ctx_.get(), &out[0], &len);
    assert(res == 1);
    assert(size == len);
    if (reuse) {
      hash_init();
    } else {
      ctx_.reset(nullptr);
    }
  }

  static void hash_oneshot(vk::span<const uint8_t> data, vk::span<uint8_t> out) noexcept {
    const EVP_MD *algorithm = digest_fetcher();
    const auto size = EVP_MD_size(algorithm);
    assert(size <= out.size());
    unsigned int len = 0;
    auto res = EVP_Digest(data.data(), data.size(), &out[0], &len, algorithm, nullptr);
    assert(res == 1);
    assert(size == len);
  }

  int result_size() noexcept {
    assert(ctx_ != nullptr);
    return EVP_MD_CTX_size(ctx_.get());
  }

private:
  vk::unique_ptr_with_delete_function<EVP_MD_CTX, EVP_MD_CTX_free> ctx_{nullptr};
};

#ifndef OPENSSL_NO_MD5
static inline void md5(vk::span<const uint8_t> data, vk::span<uint8_t> out) noexcept {
  digest<EVP_md5>::hash_oneshot(data, out);
}
#endif

static inline void sha1(vk::span<const uint8_t> data, vk::span<uint8_t> out) noexcept {
  digest<EVP_sha1>::hash_oneshot(data, out);
}

static inline void sha256(vk::span<const uint8_t> data, vk::span<uint8_t> out) noexcept {
  digest<EVP_sha256>::hash_oneshot(data, out);
}

static void inline sha512(vk::span<const uint8_t> data, vk::span<uint8_t> out) noexcept {
  digest<EVP_sha512>::hash_oneshot(data, out);
}

} // namespace vk
