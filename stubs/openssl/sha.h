#ifndef STUB_OPENSSL_SHA_H
#define STUB_OPENSSL_SHA_H
#include <stdint.h>
#include <string.h>
#define SHA_DIGEST_LENGTH 20
typedef struct { uint8_t dummy[128]; } SHA_CTX;
static inline int SHA1_Init(SHA_CTX* ctx) { memset(ctx, 0, sizeof(*ctx)); return 1; }
static inline int SHA1_Update(SHA_CTX* ctx, const void* data, size_t len) { (void)ctx; (void)data; (void)len; return 1; }
static inline int SHA1_Final(uint8_t* md, SHA_CTX* ctx) { memset(md, 0, SHA_DIGEST_LENGTH); (void)ctx; return 1; }
#endif
