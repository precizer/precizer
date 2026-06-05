/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

#include "sha512.h"
#include <stdio.h>

/* the K array */
static const uint64_t K[80] =
{
	UINT64_C(0x428a2f98d728ae22),UINT64_C(0x7137449123ef65cd),
	UINT64_C(0xb5c0fbcfec4d3b2f),UINT64_C(0xe9b5dba58189dbbc),
	UINT64_C(0x3956c25bf348b538),UINT64_C(0x59f111f1b605d019),
	UINT64_C(0x923f82a4af194f9b),UINT64_C(0xab1c5ed5da6d8118),
	UINT64_C(0xd807aa98a3030242),UINT64_C(0x12835b0145706fbe),
	UINT64_C(0x243185be4ee4b28c),UINT64_C(0x550c7dc3d5ffb4e2),
	UINT64_C(0x72be5d74f27b896f),UINT64_C(0x80deb1fe3b1696b1),
	UINT64_C(0x9bdc06a725c71235),UINT64_C(0xc19bf174cf692694),
	UINT64_C(0xe49b69c19ef14ad2),UINT64_C(0xefbe4786384f25e3),
	UINT64_C(0x0fc19dc68b8cd5b5),UINT64_C(0x240ca1cc77ac9c65),
	UINT64_C(0x2de92c6f592b0275),UINT64_C(0x4a7484aa6ea6e483),
	UINT64_C(0x5cb0a9dcbd41fbd4),UINT64_C(0x76f988da831153b5),
	UINT64_C(0x983e5152ee66dfab),UINT64_C(0xa831c66d2db43210),
	UINT64_C(0xb00327c898fb213f),UINT64_C(0xbf597fc7beef0ee4),
	UINT64_C(0xc6e00bf33da88fc2),UINT64_C(0xd5a79147930aa725),
	UINT64_C(0x06ca6351e003826f),UINT64_C(0x142929670a0e6e70),
	UINT64_C(0x27b70a8546d22ffc),UINT64_C(0x2e1b21385c26c926),
	UINT64_C(0x4d2c6dfc5ac42aed),UINT64_C(0x53380d139d95b3df),
	UINT64_C(0x650a73548baf63de),UINT64_C(0x766a0abb3c77b2a8),
	UINT64_C(0x81c2c92e47edaee6),UINT64_C(0x92722c851482353b),
	UINT64_C(0xa2bfe8a14cf10364),UINT64_C(0xa81a664bbc423001),
	UINT64_C(0xc24b8b70d0f89791),UINT64_C(0xc76c51a30654be30),
	UINT64_C(0xd192e819d6ef5218),UINT64_C(0xd69906245565a910),
	UINT64_C(0xf40e35855771202a),UINT64_C(0x106aa07032bbd1b8),
	UINT64_C(0x19a4c116b8d2d0c8),UINT64_C(0x1e376c085141ab53),
	UINT64_C(0x2748774cdf8eeb99),UINT64_C(0x34b0bcb5e19b48a8),
	UINT64_C(0x391c0cb3c5c95a63),UINT64_C(0x4ed8aa4ae3418acb),
	UINT64_C(0x5b9cca4f7763e373),UINT64_C(0x682e6ff3d6b2b8a3),
	UINT64_C(0x748f82ee5defb2fc),UINT64_C(0x78a5636f43172f60),
	UINT64_C(0x84c87814a1f0ab72),UINT64_C(0x8cc702081a6439ec),
	UINT64_C(0x90befffa23631e28),UINT64_C(0xa4506cebde82bde9),
	UINT64_C(0xbef9a3f7b2c67915),UINT64_C(0xc67178f2e372532b),
	UINT64_C(0xca273eceea26619c),UINT64_C(0xd186b8c721c0c207),
	UINT64_C(0xeada7dd6cde0eb1e),UINT64_C(0xf57d4f7fee6ed178),
	UINT64_C(0x06f067aa72176fba),UINT64_C(0x0a637dc5a2c898a6),
	UINT64_C(0x113f9804bef90dae),UINT64_C(0x1b710b35131c471b),
	UINT64_C(0x28db77f523047d84),UINT64_C(0x32caab7b40c72493),
	UINT64_C(0x3c9ebe0a15c9bebc),UINT64_C(0x431d67c49c100d4c),
	UINT64_C(0x4cc5d4becb3e42b6),UINT64_C(0x597f299cfc657e2a),
	UINT64_C(0x5fcb6fab3ad6faec),UINT64_C(0x6c44198c4a475817)
};

/* Various logical functions */

#define ROR64c(x,y) \
	( ((((x)&UINT64_C(0xFFFFFFFFFFFFFFFF))>>((uint64_t)(y)&UINT64_C(63))) | \
	((x)<<(((uint64_t)64-((uint64_t)(y)&UINT64_C(63)))&UINT64_C(63))))&UINT64_C(0xFFFFFFFFFFFFFFFF))

#define STORE64H(x,y) \
	{ \
		(y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned char)(((x)>>48)&255); \
		(y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned char)(((x)>>32)&255); \
		(y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned char)(((x)>>16)&255); \
		(y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); \
	}

#define LOAD64H(x,y) \
	{ \
		x = (((uint64_t)((y)[0] & 255))<<56)|(((uint64_t)((y)[1] & 255))<<48) | \
			(((uint64_t)((y)[2] & 255))<<40)|(((uint64_t)((y)[3] & 255))<<32) | \
			(((uint64_t)((y)[4] & 255))<<24)|(((uint64_t)((y)[5] & 255))<<16) | \
			(((uint64_t)((y)[6] & 255))<<8)|(((uint64_t)((y)[7] & 255))); \
	}

#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y)&z) | (x & y))
#define S(x,n)         ROR64c(x,n)
#define R(x,n)         (((x)&UINT64_C(0xFFFFFFFFFFFFFFFF))>>((uint64_t)n))
#define Sigma0(x)       (S(x,28) ^ S(x,34) ^ S(x,39))
#define Sigma1(x)       (S(x,14) ^ S(x,18) ^ S(x,41))
#define Gamma0(x)       (S(x,1) ^ S(x,8) ^ R(x,7))
#define Gamma1(x)       (S(x,19) ^ S(x,61) ^ R(x,6))
#ifndef MIN
#define MIN(x,y) ( ((x)<(y))?(x):(y) )
#endif

#define SHA512_BITS_PER_BYTE UINT64_C(8)
#define SHA512_BLOCK_BYTES 128U
#define SHA512_LENGTH_FIELD_OFFSET 120U
#define SHA512_MAX_BIT_COUNT UINT64_C(0xffffffffffffffff)

/**
 * @brief Mix one full SHA-512 block into the current hash state
 * @details The caller must pass a valid context and exactly one 128-byte block.
 * This routine performs only deterministic SHA-512 block arithmetic, so it has
 * no runtime failure state of its own
 *
 * @param md Hash context that receives the updated state
 * @param buf One complete 128-byte message block
 */
static void sha512_compress(
	SHA512_Context      *md,
	const unsigned char *buf)
{
	uint64_t S[8],W[80];
	int i;

	/* copy state into S */
	for(i = 0; i < 8; i++)
	{
		S[i] = md->state[i];
	}

	/* copy the state into 1024-bits into W[0..15] */
	for(i = 0; i < 16; i++)
	{
		LOAD64H(W[i],buf + (8*i));
	}

	/* fill W[16..79] */
	for(i = 16; i < 80; i++)
	{
		W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
	}

	/* Compress */
	#define RND(a,b,c,d,e,f,g,h,i) \
		t0 = h + Sigma1(e) + Ch(e,f,g) + K[i] + W[i]; \
		t1 = Sigma0(a) + Maj(a,b,c); \
		d += t0; \
		h = t0 + t1;

	for(i = 0; i < 80; i += 8)
	{
		uint64_t t0,t1;

		RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i+0);
		RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],i+1);
		RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],i+2);
		RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],i+3);
		RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],i+4);
		RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],i+5);
		RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],i+6);
		RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],i+7);
	}

	#undef RND

	/* feedback */
	for(i = 0; i < 8; i++)
	{
		md->state[i] = md->state[i] + S[i];
	}
}

/**
 * @brief Check whether more message bytes fit into the recorded SHA-512 length
 * @details This library records the message length in a 64-bit bit counter.
 * The helper keeps update and finalization code from silently wrapping that
 * counter when a caller passes an extremely large or damaged context
 *
 * @param md Hash context whose current length should be checked
 * @param inlen Number of new bytes the caller wants to add
 * @return CRYPT_OK when the bytes fit, otherwise a specific libsha512 error
 */
static SHA512_Status sha512_check_available_bytes(
	const SHA512_Context *md,
	size_t               inlen)
{
	uint64_t available_bytes;

	if(md->curlen >= sizeof(md->buf))
	{
		return CRYPT_INVALID_ARG;
	}

	available_bytes = (SHA512_MAX_BIT_COUNT - md->length) / SHA512_BITS_PER_BYTE;

	if(available_bytes < (uint64_t)md->curlen)
	{
		return CRYPT_HASH_OVERFLOW;
	}

	available_bytes -= (uint64_t)md->curlen;

	if((uint64_t)inlen > available_bytes)
	{
		return CRYPT_HASH_OVERFLOW;
	}

	return CRYPT_OK;
}

/**
 * @brief Prepare a SHA-512 context for a new message
 * @details The context is reset to the standard SHA-512 initial state and can
 * then be passed to sha512_update() and sha512_final()
 *
 * @param md Context to initialize
 * @return CRYPT_OK on success, CRYPT_INVALID_ARG when @p md is NULL
 */
SHA512_Status sha512_init(SHA512_Context *md)
{
	if(md == NULL)
	{
		return CRYPT_INVALID_ARG;
	}

	md->curlen = 0;
	md->length = 0;
	md->state[0] = UINT64_C(0x6a09e667f3bcc908);
	md->state[1] = UINT64_C(0xbb67ae8584caa73b);
	md->state[2] = UINT64_C(0x3c6ef372fe94f82b);
	md->state[3] = UINT64_C(0xa54ff53a5f1d36f1);
	md->state[4] = UINT64_C(0x510e527fade682d1);
	md->state[5] = UINT64_C(0x9b05688c2b3e6c1f);
	md->state[6] = UINT64_C(0x1f83d9abfb41bd6b);
	md->state[7] = UINT64_C(0x5be0cd19137e2179);

	return CRYPT_OK;
}

/**
 * @brief Add message bytes to a SHA-512 context
 * @details The input may be any size, including zero bytes. The function
 * buffers partial blocks internally and rejects invalid pointers, damaged
 * buffer state, or input that would overflow the recorded message length
 *
 * @param md Context that already passed through sha512_init()
 * @param in Message bytes to add to the hash
 * @param inlen Number of bytes available at @p in
 * @return CRYPT_OK on success, otherwise a specific libsha512 error
 */
SHA512_Status sha512_update(
	SHA512_Context      *md,
	const unsigned char *in,
	size_t              inlen)
{
	SHA512_Status status;
	size_t i;

	if(md == NULL)
	{
		return CRYPT_INVALID_ARG;
	}

	if(in == NULL)
	{
		return CRYPT_INVALID_ARG;
	}

	status = sha512_check_available_bytes(md,inlen);

	if(status != CRYPT_OK)
	{
		return status;
	}

	while(inlen > 0)
	{
		if(md->curlen == 0 && inlen >= SHA512_BLOCK_BYTES)
		{
			sha512_compress(md,in);
			md->length += SHA512_BLOCK_BYTES * SHA512_BITS_PER_BYTE;
			in += SHA512_BLOCK_BYTES;
			inlen -= SHA512_BLOCK_BYTES;
		} else {
			size_t n = MIN(inlen,(SHA512_BLOCK_BYTES - md->curlen));

			for(i = 0; i < n; i++)
			{
				md->buf[i + md->curlen] = in[i];
			}

			md->curlen += n;
			in += n;
			inlen -= n;

			if(md->curlen == SHA512_BLOCK_BYTES)
			{
				sha512_compress(md,md->buf);
				md->length += SHA512_BITS_PER_BYTE * SHA512_BLOCK_BYTES;
				md->curlen = 0;
			}
		}
	}
	return CRYPT_OK;
}

/**
 * @brief Finish a SHA-512 message and write its digest
 * @details Finalization adds SHA-512 padding, writes the 64-byte digest, and
 * rejects invalid output storage or a context whose buffered bytes would
 * overflow the recorded message length
 *
 * @param md Context that contains the message accumulated so far
 * @param out Destination for the 64-byte SHA-512 digest
 * @return CRYPT_OK on success, otherwise a specific libsha512 error
 */
SHA512_Status sha512_final(
	SHA512_Context *md,
	unsigned char  *out)
{
	SHA512_Status status;
	int i;

	if(md == NULL)
	{
		return CRYPT_INVALID_ARG;
	}

	if(out == NULL)
	{
		return CRYPT_INVALID_ARG;
	}

	status = sha512_check_available_bytes(md,0U);

	if(status != CRYPT_OK)
	{
		return status;
	}

	/* increase the length of the message */
	md->length += (uint64_t)md->curlen * SHA512_BITS_PER_BYTE;

	/* append the '1' bit */
	md->buf[md->curlen++] = (unsigned char)0x80;

	/* if the length is currently above 112 bytes we append zeros
	 * then compress.  Then we can fall back to padding zeros and length
	 * encoding like normal.
	 */
	if(md->curlen > 112U)
	{
		while(md->curlen < SHA512_BLOCK_BYTES)
		{
			md->buf[md->curlen++] = (unsigned char)0;
		}
		sha512_compress(md,md->buf);
		md->curlen = 0;
	}

	/* pad upto 120 bytes of zeroes
	 * note: that from 112 to 120 is the 64 MSB of the length.  We assume that you won't hash
	 * > 2^64 bits of data... :-)
	 */
	while(md->curlen < SHA512_LENGTH_FIELD_OFFSET)
	{
		md->buf[md->curlen++] = (unsigned char)0;
	}

	/* store length */
	STORE64H(md->length,md->buf + SHA512_LENGTH_FIELD_OFFSET);
	sha512_compress(md,md->buf);

	/* copy output */
	for(i = 0; i < 8; i++)
	{
		STORE64H(md->state[i],out+(8*i));
	}

	return CRYPT_OK;
}

#if 0
void test(
	const unsigned char *message,
	size_t              message_len,
	unsigned char       *out)
{
	// int ret;
	// if ((ret = sha512_init(&ctx))) return ret;
	// if ((ret = sha512_update(&ctx, message, message_len))) return ret;
	// if ((ret = sha512_final(&ctx, out))) return ret;

	SHA512_Context ctx;
	sha512_init(&ctx);
	sha512_update(&ctx,message,message_len);
	sha512_final(&ctx,out);
}

int main(void)
{
	unsigned char out[SHA512_DIGEST_LENGTH];
	const unsigned char message[] = "Hello World";
	test(message,sizeof(message),out);

	for(size_t i = 0; i < 64; i++)
	{
		printf("%02x",out[i]);
	}
	putchar('\n');

	return(0);
}
#endif
