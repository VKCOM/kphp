// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC Sigmalab
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "runtime/kphp_core.h"
#include <sys/types.h>

bool f$touch(const string filename, const int64_t mtime=0, const int64_t atime=0);
