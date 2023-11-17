/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef SHA_H
#define	SHA_H

#ifdef	__cplusplus
extern "C" {
#endif

/*************************** HEADER FILES ***************************/
#include <stddef.h>
#include <xc.h>    
    
/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef uint32_t  SHA_WORD;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
	uint8_t data[64];
	SHA_WORD datalen;
	unsigned long long bitlen;
	SHA_WORD state[8];
} SHA256_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[]);

#ifdef	__cplusplus
}
#endif

#endif	/* SHA_H */

