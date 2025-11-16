// Credits to https://github.com/Zunawe for this MD5 implementation : https://github.com/Zunawe/md5-c

#ifndef ARION_MD5_HPP
#define ARION_MD5_HPP

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MD5_DIGEST_LEN 16

#define ARION_MD5_A 0x67452301
#define ARION_MD5_B 0xefcdab89
#define ARION_MD5_C 0x98badcfe
#define ARION_MD5_D 0x10325476

#define ARION_MD5_F(X, Y, Z) ((X & Y) | (~X & Z))
#define ARION_MD5_G(X, Y, Z) ((X & Z) | (Y & ~Z))
#define ARION_MD5_H(X, Y, Z) (X ^ Y ^ Z)
#define ARION_MD5_I(X, Y, Z) (Y ^ (X | ~Z))

namespace arion
{

/// This structure holds the context for MD5 calculation.
struct MD5_CTXT
{
    /// Total size of the input data in bits.
    uint64_t size;
    /// The four 32-bit buffers/registers (A, B, C, D) used in the hash calculation.
    uint32_t buffer[4];
    /// Temporary input buffer (64 bytes or 512 bits).
    uint8_t input[64];
    /// The resulting 16-byte (128-bit) MD5 digest.
    uint8_t digest[16];
};

/// Array of shift amounts S[i] used in the MD5 algorithm.
static uint32_t S[] = {7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
                       14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
                       4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21};

/// Array of 64 pre-calculated sine constants K[i] used in the MD5 algorithm.
static uint32_t K[] = {0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                       0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                       0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                       0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                       0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                       0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                       0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                       0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

/// The MD5 padding constant array.
static uint8_t PADDING[] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/**
 * Performs a left bitwise rotation on a 32-bit word.
 * @param[in] x The 32-bit word to rotate.
 * @param[in] n The number of bits to rotate left by.
 * @return The result of the left rotation.
 */
inline uint32_t rotate_left(uint32_t x, uint32_t n)
{
    return (x << n) | (x >> (32 - n));
}

/**
 * Initializes the MD5 context. Sets the size to 0 and the buffers (A, B, C, D) to their initial values.
 * @param[in] ctx Pointer to the MD5 context structure.
 */
inline void md5_init(MD5_CTXT *ctx)
{
    ctx->size = (uint64_t)0;

    ctx->buffer[0] = (uint32_t)ARION_MD5_A;
    ctx->buffer[1] = (uint32_t)ARION_MD5_B;
    ctx->buffer[2] = (uint32_t)ARION_MD5_C;
    ctx->buffer[3] = (uint32_t)ARION_MD5_D;
}

/**
 * Performs a single MD5 transformation step on a 64-byte block.
 * @param[in,out] buffer The current state of the four 32-bit buffers.
 * @param[in] input The 16 32-bit words (64 bytes) of the block to process.
 */
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
            E = ARION_MD5_F(BB, CC, DD);
            j = i;
            break;
        case 1:
            E = ARION_MD5_G(BB, CC, DD);
            j = ((i * 5) + 1) % 16;
            break;
        case 2:
            E = ARION_MD5_H(BB, CC, DD);
            j = ((i * 3) + 5) % 16;
            break;
        default:
            E = ARION_MD5_I(BB, CC, DD);
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

/**
 * Updates the MD5 context with a portion of the input data.
 * Processes full 64-byte blocks and updates the context's total size.
 * @param[in,out] ctx Pointer to the MD5 context structure.
 * @param[in] input_buffer Pointer to the input data buffer.
 * @param[in] input_len The length of the input data in bytes.
 */
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

/**
 * Finalizes the MD5 calculation. Applies padding, processes the last block, and stores the digest.
 * @param[in,out] ctx Pointer to the MD5 context structure. The final digest is stored in ctx->digest.
 */
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

/**
 * Calculates the MD5 hash of a null-terminated string.
 * @param[in] input The null-terminated C string to hash.
 * @param[out] result Pointer to a 16-byte array where the MD5 digest will be stored.
 */
inline void md5_string(char *input, uint8_t *result)
{
    MD5_CTXT ctx;
    md5_init(&ctx);
    md5_update(&ctx, (uint8_t *)input, strlen(input));
    md5_finalize(&ctx);

    memcpy(result, ctx.digest, 16);
}

/**
 * Calculates the MD5 hash of a file's content.
 * The file stream must be open and readable.
 * @param[in] file Pointer to the open FILE stream.
 * @param[out] result Pointer to a 16-byte array where the MD5 digest will be stored.
 */
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

}; // namespace arion

#endif // ARION_MD5_HPP
