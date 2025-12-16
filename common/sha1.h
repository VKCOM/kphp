// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "openssl/sha.h"

void sha1(unsigned char* input, int ilen, unsigned char output[20]);
