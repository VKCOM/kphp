// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_SIMD_VECTOR_H
#define KDB_COMMON_SIMD_VECTOR_H

typedef long long v2di __attribute__((vector_size(16)));
typedef char v16qi __attribute__((vector_size(16)));
typedef short v8hi __attribute__((vector_size(16)));
typedef int v4si __attribute__((vector_size(16)));

#endif // KDB_COMMON_SIMD_VECTOR_H
