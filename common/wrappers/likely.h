// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#ifndef likely
#define likely(x) __builtin_expect ((x),1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect ((x),0)
#endif
