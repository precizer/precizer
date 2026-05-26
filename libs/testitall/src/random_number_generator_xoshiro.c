/* The following license applies to the original xoshiro256** 1.0 library
   published at https://prng.di.unimi.it/xoshiro256starstar.c and the original
   SplitMix64 implementation published at https://prng.di.unimi.it/splitmix64.c.
   Their code was adapted here for integer random-number generation needs */

/* xoshiro256** 1.0 was written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org).
   SplitMix64 was written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include "testitall.h"
#include <stdio.h>
#include <stdint.h>

/* This is xoshiro256** 1.0, one of our all-purpose, rock-solid
   generators. It has excellent (sub-ns) speed, a state (256 bits) that is
   large enough for any parallel application, and it passes all tests we
   are aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s.

   The original xoshiro256** source also documents jump helpers. This
   wrapper keeps that documentation for attribution but does not expose the
   helpers.

   The jump function for the generator is equivalent to 2^128 calls to
   next(); it can be used to generate 2^128 non-overlapping subsequences
   for parallel computations.

   The long-jump function for the generator is equivalent to 2^192 calls
   to next(); it can be used to generate 2^64 starting points, from each of
   which jump() will generate 2^64 non-overlapping subsequences for
   parallel distributed computations */
static uint64_t xoshiro256starstar_state[4] = {0U,0U,0U,0U};
static bool xoshiro256starstar_initialized = false;

static uint64_t xoshiro256starstar_rotate_left(
	uint64_t value,
	unsigned int shift) __attribute__((const));

/**
 * @brief Rotate a 64-bit value left
 *
 * @param value Value to rotate
 * @param shift Number of bits to rotate by
 * @return Rotated value
 */
static uint64_t xoshiro256starstar_rotate_left(
	uint64_t value,
	unsigned int shift)
{
	return((value << shift) | (value >> (64U - shift)));
}

/**
 * @brief Generate a SplitMix64 value and advance its state
 * @details The following comments apply to the original SplitMix64
 * implementation published at https://prng.di.unimi.it/splitmix64.c
 *
 * This is a fixed-increment version of Java 8's SplittableRandom generator.
 * See http://dx.doi.org/10.1145/2714064.2660195 and
 * http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html
 *
 * It is a very fast generator passing BigCrush, and it can be useful if
 * for some reason you absolutely want 64 bits of state. The state can be
 * seeded with any value
 *
 * SplitMix64 is used only to expand one 64-bit seed into xoshiro256** state
 *
 * @param state SplitMix64 state
 * @return Next SplitMix64 value
 */
static uint64_t splitmix64_next(uint64_t *state)
{
	uint64_t value = *state + UINT64_C(0x9E3779B97F4A7C15);
	*state = value;
	value = (value ^ (value >> 30U)) * UINT64_C(0xBF58476D1CE4E5B9);
	value = (value ^ (value >> 27U)) * UINT64_C(0x94D049BB133111EB);
	value = value ^ (value >> 31U);

	return(value);
}

/**
 * @brief Generate the next xoshiro256** value
 *
 * Based on the public-domain xoshiro256** generator by David Blackman and Sebastiano Vigna
 *
 * @return Next 64-bit pseudorandom value
 */
static uint64_t xoshiro256starstar_next(void)
{
	const uint64_t result = xoshiro256starstar_rotate_left(xoshiro256starstar_state[1] * UINT64_C(5),7U) * UINT64_C(9);
	const uint64_t transient = xoshiro256starstar_state[1] << 17U;

	xoshiro256starstar_state[2] ^= xoshiro256starstar_state[0];
	xoshiro256starstar_state[3] ^= xoshiro256starstar_state[1];
	xoshiro256starstar_state[1] ^= xoshiro256starstar_state[2];
	xoshiro256starstar_state[0] ^= xoshiro256starstar_state[3];
	xoshiro256starstar_state[2] ^= transient;
	xoshiro256starstar_state[3] = xoshiro256starstar_rotate_left(xoshiro256starstar_state[3],45U);

	return(result);
}

/**
 * @brief Initialize xoshiro256** state from /dev/urandom
 *
 * @return SUCCESS on success or FAILURE on error
 */
static Return xoshiro256starstar_initialize(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	FILE *fp = fopen("/dev/urandom","rb");

	if(NULL == fp)
	{
		echo(STDERR,"Can't open /dev/urandom\n");
		status = FAILURE;
	}

	uint64_t seed = 0U;

	if(SUCCESS == status)
	{
		if(fread(&seed,sizeof(seed),1,fp) != 1)
		{
			echo(STDERR,"Failed to read from /dev/urandom\n");
			status = FAILURE;
		}
	}

	if(NULL != fp)
	{
		fclose(fp);
	}

	if(SUCCESS == status)
	{
		uint64_t splitmix64_state = seed;

		for(size_t i = 0U; i < 4U; i++)
		{
			xoshiro256starstar_state[i] = splitmix64_next(&splitmix64_state);
		}

		if(0U == xoshiro256starstar_state[0]
		        && 0U == xoshiro256starstar_state[1]
		        && 0U == xoshiro256starstar_state[2]
		        && 0U == xoshiro256starstar_state[3])
		{
			xoshiro256starstar_state[0] = UINT64_C(0x9E3779B97F4A7C15);
		}

		xoshiro256starstar_initialized = true;
	}

	deliver(status);
}

/**
 * @brief Ensure xoshiro256** is initialized before use
 *
 * @return SUCCESS on success or FAILURE on error
 */
static Return xoshiro256starstar_ensure_initialized(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(false == xoshiro256starstar_initialized)
	{
		status = xoshiro256starstar_initialize();
	}

	deliver(status);
}

/**
 * @brief Map xoshiro256** output to an inclusive range without modulo bias
 *
 * @param random_number Output random value
 * @param start Inclusive lower bound
 * @param end Inclusive upper bound
 */
static void xoshiro256starstar_bounded(
	uint64_t *random_number,
	uint64_t start,
	uint64_t end)
{
	if(0U == start && UINT64_MAX == end)
	{
		*random_number = xoshiro256starstar_next();
	} else {
		const uint64_t range = (end - start) + 1U;
		const uint64_t threshold = (UINT64_MAX - range + 1U) % range;
		uint64_t value = xoshiro256starstar_next();

		while(value < threshold)
		{
			value = xoshiro256starstar_next();
		}

		*random_number = start + (value % range);
	}
}

/**
 * @brief Generate a pseudorandom integer in the inclusive `[start..end]` range
 *
 * The generator uses xoshiro256** after one-time seeding from /dev/urandom
 * It is not cryptographically secure
 *
 * @param[out] random_number Output random value
 * @param[in] start Inclusive lower bound
 * @param[in] end Inclusive upper bound
 * @return SUCCESS on success or FAILURE on error
 */
Return random_number_generator_xoshiro(
	uint64_t *random_number,
	uint64_t start,
	uint64_t end)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(NULL == random_number)
	{
		echo(STDERR,"Invalid random number output pointer\n");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(end < start)
		{
			echo(STDERR,"Invalid range: end < start\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = xoshiro256starstar_ensure_initialized();
	}

	if(SUCCESS == status)
	{
		xoshiro256starstar_bounded(random_number,start,end);
	}

	deliver(status);
}
