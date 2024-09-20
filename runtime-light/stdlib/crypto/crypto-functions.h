//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/tl/tl-types.h"

task_t<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) noexcept;
task_t<Optional<array<mixed>>> f$openssl_x509_parse(const string &data, bool shortnames = true) noexcept;
task_t<bool> f$openssl_sign(const string &data, string &signature, const string &private_key, int64_t algo=tl::DigestAlgorithm::SHA1) noexcept;
task_t<int64_t> f$openssl_verify(const string &data, const string &signature, const string &pub_key_id, int64_t algo=tl::DigestAlgorithm::SHA1) noexcept;
