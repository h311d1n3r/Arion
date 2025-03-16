// Credits to https://github.com/Zunawe for this MD5 implementation : https://github.com/Zunawe/md5-c

#ifndef ARION_MD5_HPP
#define ARION_MD5_HPP

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MD5_DIGEST_LEN 16

#define A 0x67452301
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476

struct MD5_CTXT
{
    uint64_t size;
    uint32_t buffer[4];
    uint8_t input[64];
    uint8_t digest[16];
};

static uint32_t S[] = {7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
                       14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
                       4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21};

static uint32_t K[] = {0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                       0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                       0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                       0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                       0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                       0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                       0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                       0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

static uint8_t PADDING[] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define F(X, Y, Z) ((X & Y) | (~X & Z))
#define G(X, Y, Z) ((X & Z) | (Y & ~Z))
#define H(X, Y, Z) (X ^ Y ^ Z)
#define I(X, Y, Z) (Y ^ (X | ~Z))

inline uint32_t rotate_left(uint32_t x, uint32_t n)
{
    return (x << n) | (x >> (32 - n));
}

inline void md5_init(MD5_CTXT *ctx)
{
    ctx->size = (uint64_t)0;

    ctx->buffer[0] = (uint32_t)A;
    ctx->buffer[1] = (uint32_t)B;
    ctx->buffer[2] = (uint32_t)C;
    ctx->buffer[3] = (uint32_t)D;
}

inline void md5_step(uint32_t *buffer, uint32_t *input)
{
    uint32_t AA = buffer[0];
    uint32_t BB = buffer[1];
    uint32_t CC = buffer[2];
    uint32_t DD = buffer[3];

    uint32_t E;

    unsigned int j;

    for (unsigned int i = 0; i < 64; ++i)
    {
        switch (i / 16)
        {
        case 0:
            E = F(BB, CC, DD);
            j = i;
            break;
        case 1:
            E = G(BB, CC, DD);
            j = ((i * 5) + 1) % 16;
            break;
        case 2:
            E = H(BB, CC, DD);
            j = ((i * 3) + 5) % 16;
            break;
        default:
            E = I(BB, CC, DD);
            j = (i * 7) % 16;
            break;
        }

        uint32_t temp = DD;
        DD = CC;
        CC = BB;
        BB = BB + rotate_left(AA + E + K[i] + input[j], S[i]);
        AA = temp;
    }

    buffer[0] += AA;
    buffer[1] += BB;
    buffer[2] += CC;
    buffer[3] += DD;
}

inline void md5_update(MD5_CTXT *ctx, uint8_t *input_buffer, size_t input_len)
{
    uint32_t input[16];
    unsigned int offset = ctx->size % 64;
    ctx->size += (uint64_t)input_len;

    for (unsigned int i = 0; i < input_len; ++i)
    {
        ctx->input[offset++] = (uint8_t)*(input_buffer + i);

        if (offset % 64 == 0)
        {
            for (unsigned int j = 0; j < 16; ++j)
            {
                input[j] = (uint32_t)(ctx->input[(j * 4) + 3]) << 24 | (uint32_t)(ctx->input[(j * 4) + 2]) << 16 |
                           (uint32_t)(ctx->input[(j * 4) + 1]) << 8 | (uint32_t)(ctx->input[(j * 4)]);
            }
            md5_step(ctx->buffer, input);
            offset = 0;
        }
    }
}

inline void md5_finalize(MD5_CTXT *ctx)
{
    uint32_t input[16];
    unsigned int offset = ctx->size % 64;
    unsigned int padding_length = offset < 56 ? 56 - offset : (56 + 64) - offset;

    md5_update(ctx, PADDING, padding_length);
    ctx->size -= (uint64_t)padding_length;

    for (unsigned int j = 0; j < 14; ++j)
    {
        input[j] = (uint32_t)(ctx->input[(j * 4) + 3]) << 24 | (uint32_t)(ctx->input[(j * 4) + 2]) << 16 |
                   (uint32_t)(ctx->input[(j * 4) + 1]) << 8 | (uint32_t)(ctx->input[(j * 4)]);
    }
    input[14] = (uint32_t)(ctx->size * 8);
    input[15] = (uint32_t)((ctx->size * 8) >> 32);

    md5_step(ctx->buffer, input);

    for (unsigned int i = 0; i < 4; ++i)
    {
        ctx->digest[(i * 4) + 0] = (uint8_t)((ctx->buffer[i] & 0x000000FF));
        ctx->digest[(i * 4) + 1] = (uint8_t)((ctx->buffer[i] & 0x0000FF00) >> 8);
        ctx->digest[(i * 4) + 2] = (uint8_t)((ctx->buffer[i] & 0x00FF0000) >> 16);
        ctx->digest[(i * 4) + 3] = (uint8_t)((ctx->buffer[i] & 0xFF000000) >> 24);
    }
}

inline void md5_string(char *input, uint8_t *result)
{
    MD5_CTXT ctx;
    md5_init(&ctx);
    md5_update(&ctx, (uint8_t *)input, strlen(input));
    md5_finalize(&ctx);

    memcpy(result, ctx.digest, 16);
}

inline void md5_file(FILE *file, uint8_t *result)
{
    char *input_buffer = (char *)malloc(1024);
    size_t input_size = 0;

    MD5_CTXT ctx;
    md5_init(&ctx);

    while ((input_size = fread(input_buffer, 1, 1024, file)) > 0)
    {
        md5_update(&ctx, (uint8_t *)input_buffer, input_size);
    }

    md5_finalize(&ctx);

    free(input_buffer);

    memcpy(result, ctx.digest, 16);
}

#endif // ARION_MD5_HPP
