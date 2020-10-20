#ifndef __SHA1_H__
#define __SHA1_H__

#include <openssl/sha.h>

#ifdef __cplusplus
extern "C" {
#endif

void sha1 (unsigned char *input, int ilen, unsigned char output[20]);

#ifdef __cplusplus
}
#endif
#endif
