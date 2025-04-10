// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <functional>
#include <random>
#include <vector>

#include "common/crypto/aes256-generic.h"
#include "common/crypto/aes256.h"

static void BM_crypto_aes256_set_encrypt_key(benchmark::State& state) {
  std::array<std::uint8_t, 32> key;
  std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t> engine;
  std::generate(key.begin(), key.end(), std::ref(engine));

  for (auto _ : state) {
    vk_aes_ctx_t ctx;
    vk_aes_set_encrypt_key(&ctx, key.data(), AES256_KEY_BITS);
  }
}
BENCHMARK(BM_crypto_aes256_set_encrypt_key);

static void BM_crypto_aes256_set_decrypt_key(benchmark::State& state) {
  std::array<std::uint8_t, 32> key;
  std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t> engine;
  std::generate(key.begin(), key.end(), std::ref(engine));

  for (auto _ : state) {
    vk_aes_ctx_t ctx;
    vk_aes_set_decrypt_key(&ctx, key.data(), AES256_KEY_BITS);
  }
}
BENCHMARK(BM_crypto_aes256_set_decrypt_key);

static void BM_crypto_aes256_encrypt_cbc(benchmark::State& state) {
  std::array<std::uint8_t, 16> iv;
  std::array<std::uint8_t, 32> key;
  std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t> engine;
  std::generate(key.begin(), key.end(), std::ref(engine));
  std::generate(iv.begin(), iv.end(), std::ref(engine));

  const std::size_t size = state.range(0);
  std::vector<std::uint8_t> payload(size), ciphertext(size);
  std::generate(payload.begin(), payload.end(), std::ref(engine));

  vk_aes_ctx_t ctx;
  vk_aes_set_encrypt_key(&ctx, key.data(), AES256_KEY_BITS);

  for (auto _ : state) {
    ctx.cbc_crypt(&ctx, payload.data(), ciphertext.data(), payload.size(), iv.data());
  }
}
BENCHMARK(BM_crypto_aes256_encrypt_cbc)->RangeMultiplier(2)->Range(16, 16 << 20);

static void BM_crypto_generic_aes256_encrypt_cbc(benchmark::State& state) {
  std::array<std::uint8_t, 32> key, iv;
  std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t> engine;
  std::generate(key.begin(), key.end(), std::ref(engine));
  std::generate(iv.begin(), iv.end(), std::ref(engine));

  const std::size_t size = state.range(0);
  std::vector<std::uint8_t> payload(size), ciphertext(size);
  std::generate(payload.begin(), payload.end(), std::ref(engine));

  vk_aes_ctx_t ctx;
  crypto_generic_aes256_set_encrypt_key(&ctx, key.data());

  for (auto _ : state) {
    crypto_generic_aes256_cbc_encrypt(&ctx, payload.data(), ciphertext.data(), payload.size(), iv.data());
  }
}
BENCHMARK(BM_crypto_generic_aes256_encrypt_cbc)->RangeMultiplier(2)->Range(16, 16 << 20);

static void BM_crypto_generic_aes256_encrypt_ige(benchmark::State& state) {
  std::array<std::uint8_t, 32> key, iv;
  std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t> engine;
  std::generate(key.begin(), key.end(), std::ref(engine));
  std::generate(iv.begin(), iv.end(), std::ref(engine));

  const std::size_t size = state.range(0);
  std::vector<std::uint8_t> payload(size), ciphertext(size);
  std::generate(payload.begin(), payload.end(), std::ref(engine));

  vk_aes_ctx_t ctx;
  crypto_generic_aes256_set_encrypt_key(&ctx, key.data());

  for (auto _ : state) {
    crypto_generic_aes256_ige_encrypt(&ctx, payload.data(), ciphertext.data(), payload.size(), iv.data());
  }
}
BENCHMARK(BM_crypto_generic_aes256_encrypt_ige)->RangeMultiplier(2)->Range(16, 16 << 20);

static void BM_crypto_aes256_encrypt_ige(benchmark::State& state) {
  std::array<std::uint8_t, 32> key, iv;
  std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t> engine;
  std::generate(key.begin(), key.end(), std::ref(engine));
  std::generate(iv.begin(), iv.end(), std::ref(engine));

  const std::size_t size = state.range(0);
  std::vector<std::uint8_t> payload(size), ciphertext(size);
  std::generate(payload.begin(), payload.end(), std::ref(engine));

  vk_aes_ctx_t ctx;
  vk_aes_set_encrypt_key(&ctx, key.data(), AES256_KEY_BITS);

  for (auto _ : state) {
    ctx.ige_crypt(&ctx, payload.data(), ciphertext.data(), payload.size(), iv.data());
  }
}
BENCHMARK(BM_crypto_aes256_encrypt_ige)->RangeMultiplier(2)->Range(16, 16 << 20);

static void BM_crypto_aes256_encrypt_ctr(benchmark::State& state) {
  std::array<std::uint8_t, 16> iv;
  std::array<std::uint8_t, 32> key;
  std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t> engine;
  std::generate(key.begin(), key.end(), std::ref(engine));
  std::generate(iv.begin(), iv.end(), std::ref(engine));

  const std::size_t size = state.range(0);
  std::vector<std::uint8_t> payload(size), ciphertext(size);
  std::generate(payload.begin(), payload.end(), std::ref(engine));

  vk_aes_ctx_t ctx;
  vk_aes_set_encrypt_key(&ctx, key.data(), AES256_KEY_BITS);

  for (auto _ : state) {
    ctx.ctr_crypt(&ctx, payload.data(), ciphertext.data(), payload.size(), iv.data(), 0);
  }
}
BENCHMARK(BM_crypto_aes256_encrypt_ctr)->RangeMultiplier(2)->Range(16, 16 << 20);

BENCHMARK_MAIN();
