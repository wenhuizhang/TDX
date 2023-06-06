// Intel Proprietary 
// 
// Copyright 2021 Intel Corporation All Rights Reserved.
// 
// Your use of this software is governed by the TDX Source Code LIMITED USE LICENSE.
// 
// The Materials are provided “as is,” without any express or implied warranty of any kind including warranties
// of merchantability, non-infringement, title, or fitness for a particular purpose.

/**
 * @file sha384.h
 * @brief Crypto API for SHA384 Hash
 */
#ifndef __SHA384_H_INCLUDED__
#define __SHA384_H_INCLUDED__


#include "tdx_basic_defs.h"
#include "tdx_basic_types.h"

typedef int32_t crypto_api_error;
#define SIZE_OF_SHA384_BLOCK_IN_QWORD 16
#define SIZE_OF_SHA384_BLOCK_IN_DWORD (SIZE_OF_SHA384_BLOCK_IN_QWORD<<1)
#define SIZE_OF_SHA384_BLOCK_IN_BYTES (SIZE_OF_SHA384_BLOCK_IN_DWORD<<2)
#define SIZE_OF_SHA384_STATE_IN_QWORD 8
#define SIZE_OF_SHA384_STATE_IN_DWORD (SIZE_OF_SHA384_STATE_IN_QWORD<<1)
#define SIZE_OF_SHA384_STATE_IN_BYTES (SIZE_OF_SHA384_STATE_IN_DWORD<<2)
#define SIZE_OF_SHA384_HASH_IN_QWORDS 6
#define SIZE_OF_SHA384_HASH_IN_BYTES (SIZE_OF_SHA384_HASH_IN_QWORDS << 3)

#define HASH_METHOD_BUFFER_SIZE       128
#define SIZE_OF_SHA384_CTX_BUFFER     256

typedef struct hash_method_s
{
    uint8_t hash_method_buffer[HASH_METHOD_BUFFER_SIZE];
    bool_t  is_initialized;
} hash_method_t;

typedef union measurement_u
{
    uint64_t qwords[SIZE_OF_SHA384_HASH_IN_QWORDS];
    uint8_t  bytes[SIZE_OF_SHA384_HASH_IN_BYTES];
} measurement_t;
tdx_static_assert(sizeof(measurement_t) == SIZE_OF_SHA384_HASH_IN_BYTES, measurement_t);

/**
 * @struct sha384_ctx_t
 *
 * @brief Context of an incremental SHA384 process.
 */
typedef struct sha384_ctx_s
{
    uint8_t buffer[SIZE_OF_SHA384_CTX_BUFFER];
} sha384_ctx_t;


/**
 * @struct sha384_128B_block_t
 *
 * @brief Standard block size for a SHA384 hash - 128 Bytes.
 */
typedef struct PACKED sha384_128B_block_s
{
    union
    {
        uint64_t block_qword_buffer[SIZE_OF_SHA384_BLOCK_IN_QWORD];
        uint32_t block_dword_buffer[SIZE_OF_SHA384_BLOCK_IN_DWORD];
        uint8_t  block_byte_buffer[SIZE_OF_SHA384_BLOCK_IN_BYTES];

        struct // mr_td_extend_struct
        {
            /**
             * SEAMCALL API function name, as a string of 8-bit ASCII characters,
             * padded with 0’s
             */
            uint128_t api_name;
            uint64_t  gpa; /** < Guest physical address */
            uint8_t   reserved[SIZE_OF_SHA384_BLOCK_IN_BYTES - 24 /* api_name + gpa */]; /** < Must be 0 */
        };
    };
} sha384_128B_block_t;
tdx_static_assert(sizeof(sha384_128B_block_t) == SIZE_OF_SHA384_BLOCK_IN_BYTES, sha384_128B_block_t);


/**
 * @brief Initialize an incremental SHA384 hash creation
 *
 * @note
 *
 * @param ctx Context of incremental SHA384 hash process
 *
 * @return Success or Error type
 */
crypto_api_error sha384_init(sha384_ctx_t * ctx);


/**
 * @brief Update an incremental SHA384 hash by N number of blocks
 *
 * @note
 *
 * @param ctx Context of incremental SHA384 hash process
 * @param blocks Blocks to process in the SHA384 hash (each of size 128 byte)
 * @param num_of_blocks Number of blocks in the blocks array
 *
 * @return Success or Error type
 */
crypto_api_error sha384_update_128B(sha384_ctx_t * ctx,
                                    const sha384_128B_block_t * blocks,
                                    uint32_t num_of_blocks);


/**
 * @brief Finalize the SHA384 incremental process and get the final result
 *
 * @note
 *
 * @param ctx Context of incremental SHA384 hash process
 * @param hash Result of SHA384 hash
 *
 * @return Success or Error type
 */
crypto_api_error sha384_finalize(sha384_ctx_t * ctx, uint64_t * hash);


/**
 * @brief Non-incremental SHA-384 hash. Receives num_of_blocks of 128 byte and calculates the hash
 *
 * @note
 *
 * @param blocks Blocks to process in the SHA384 hash (each of size 128 byte)
 * @param num_of_blocks Number of blocks in the blocks array
 * @param hash Result of SHA384 hash
 *
 * @return Success or Error type
 */
crypto_api_error sha384_generate_hash_128B(const sha384_128B_block_t * blocks,
                                           uint32_t num_of_blocks,
                                           uint64_t * hash);


/**
 * @brief Non-incremental SHA-384 hash for a block smaller than 128 bytes
 *
 * @note
 *
 * @param block Pointer to block
 * @param block_size Size of block in bytes
 * @param hash Result of SHA384 hash
 *
 * @return Success or Error type
 */
crypto_api_error sha384_generate_hash(const uint8_t * block,
                                      uint32_t block_size,
                                      uint64_t * hash);


#endif // __SHA384_H_INCLUDED__
