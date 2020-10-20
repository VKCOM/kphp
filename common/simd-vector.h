#ifndef KDB_COMMON_SIMD_VECTOR_H
#define KDB_COMMON_SIMD_VECTOR_H

typedef long long v2di __attribute__ ((vector_size (16)));
typedef char v16qi __attribute__ ((vector_size (16)));
typedef short v8hi __attribute__ ((vector_size (16)));
typedef int v4si __attribute__ ((vector_size (16)));

#endif // KDB_COMMON_SIMD_VECTOR_H
