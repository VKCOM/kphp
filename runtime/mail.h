// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/runtime-types.h"

bool f$mail(const string &to, const string &subject, const string &message, string additional_headers = string());
