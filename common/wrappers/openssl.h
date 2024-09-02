// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy

#endif
