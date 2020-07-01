/*
 * urun2.c
 *
 * @date 2020-06-29 16:25:46
 *
 * Implementation of opcodes for "unsigned runlength-2" encoding.
 *
 * @date 2020-07-01 02:53:59
 * Previous implementation had the least number of conditionals/jumps. but the bitstream overhead. oo.
 */

/*
 *	This file is part of Armonika,
 *	Encoding/decoding/handling of variable length numbers in bit addressable memory.
 *	Copyright (C) 2020, xyzzy@rockingship.org
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>

// what is value of next bit in the sequence
#define bit(MEM, POS) ({ unsigned _pos=(POS); (MEM)[_pos>>3] & (1<<(_pos&7)); })
// this is next bit of the sequence
#define emit(MEM, POS, BIT) ({ unsigned _pos=(POS); if (BIT) (MEM)[_pos>>3] |= 1<<(_pos&7); else (MEM)[_pos>>3] &= ~(1<<(_pos&7)); })

unsigned OR(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {

	// three pipelines, two for left/right operands one for the result.
	// two consecutive "0" is the trigger. Either a "0" to end the sequence or "1" to escape and continue.
	//
	unsigned lstate = 1, lbit;
	unsigned rstate = 1, rbit;
	unsigned estate = 1, ebit;

	do {
		/*
		 * Two independent and parallel 'loops' to load next bit of sequence.
		 * When runlength reached swallow escape bit.
		 * Source optimize to have the least number of lvalues.
		 */
		if (lstate) {
			lbit = bit(pL, iL++) ? 1 : 0;
			lstate = lbit ? 1 : lstate << 1;

			if (lstate == 4)
				lstate = bit(pL, iL++) ? 1 : 0; // either end-of-sequence on "0", knowing that two "0" already have neen emitted. the third is also a terminator.
		}

		if (rstate) {
			rbit = bit(pR, iR++) ? 1 : 0;
			rstate = rbit ? 1 : rstate << 1;

			if (rstate == 4)
				rstate = bit(pR, iR++) ? 1 : 0; // either end-of-sequence on "0", knowing that two "0" already have neen emitted. the third is also a terminator.
		}

		/*
		 * Escape current streak when two consecutivee "0" have already been emitted.
		 * Do this before emitting data because if data were "0" it could collapse with the end-of-sequence
		 */
		if (estate == 4) {
			emit(pDst, dstpos++, 1);
			estate = 1;
		}

		/*
		 * Operator
		 */
		ebit = lbit | rbit;
		emit(pDst, dstpos++, ebit);

		/*
		 * Emitting "1" resets the runlength counter
		 */
		estate = ebit ? 1 : estate << 1;

	} while (lstate | rstate);
	// lstate | rstate requires a merge ("|") and the vector is tested for zero.
	// lstate || rstate requires two tests.

	/*
	 * Keep emitting leading "0" until end-of-sequence complete.
	 * Leading zeros can already be in effect as part of the result.
	 */
	do {
		emit(pDst, dstpos++, 0);
	} while ( (estate <<= 1) != 8);

	return dstpos;
}

/**
 * Encode value into runlength-N, return string and length.
 * NOTE: encoded number is unsigned
 *
 * @param {string} pDest - Encoded stream as string (LSB first).
 * @param {number} num - number to encode.
 * @param {number} N - runlength
 * @return {number} - length including terminator.
 */
unsigned encode(unsigned char *pDest, unsigned bitpos, uint64_t num, int N) {
	unsigned count = 0; // current runlength (number of consecutive bits of same polarity)

	// as long as there are input bits
	while (num) {
		// extract next LSB from input
		unsigned b = num & 1;
		num >>= 1;

		// inject into output
		emit(pDest, bitpos++, b);

		// update runlength
		if (b != 0) {
			// consecutive "1" can be unlimited in length
			count = 0;
		} else if (++count == N) {
			// runlength limit reached, inject opposite polarity
			emit(pDest, bitpos++, 1);
			count = 0;
		}
	}

	// append terminator
	while (N >= 0) {
		emit(pDest, bitpos++, 0);
		--N;
	}

	return bitpos;
}

/**
 * Encode value into runlength-N, return string and length.
 * NOTE: encoded number is unsigned
 *
 * @param {string} pSource - Start of sequence as string (LSB first).
 * @param {number} N - runlength
 * @return {int64_t} - The decoded value as variable length structurele. for demonstration purpose assuming it will fit in less that 64 bits.
 */
uint64_t decode(unsigned char *pSrc, unsigned bitpos, int N) {
	unsigned count = 0; // current runlength (number of consecutive bits of same polarity)
	uint64_t mask = 1; // current bit to merge with number
	uint64_t num = 0; // number being decoded

	// The condition is a failsafe as the runN terminator is the end condition
	for (;;) {
		// extract next LSB from input
		uint64_t b = bit(pSrc, bitpos++);

		// inject into output
		if (b)
			num |= mask;
		mask <<= 1;

		// update runlength
		if (b != 0) {
			// consecutive "1" can be unlimited in length
			count = 0;
		} else if (++count == N) {
			// runlength reached, get next bit
			b = bit(pSrc, bitpos++);
			if (b == 0)
				break; // N+1 consecutive bits of same polarity is terminator
			count = 0;
		}
	}

	return num;
}

unsigned char mem[512];
unsigned pos;

int main() {

	/*
	 * Test the function by trying all 12-bit possibilities per variable for "<left> OR <right>
	 */
	unsigned lval, rval;
	for (lval=0; lval < (1<<12); lval++) {
		for (rval = 0; rval < (1 << 12); rval++) {

			// rewind memort
			pos = 0;

			// encode <left>
			unsigned iL = pos;
			pos = encode(mem, pos, lval, 2);

			// encode <right>
			unsigned iR = pos;
			pos = encode(mem, pos, rval, 2);

			// perform `OR`
			unsigned iOR = pos;
			pos = OR(mem, iOR, mem, iL, mem, iR);

			// extract
			uint64_t answer;
			answer = decode(mem, iOR, 2);

			// encode answer to determine length
			unsigned iAnswer = pos;
			pos = encode(mem, pos, answer, 2);

			/*
			 * Compare
			 */
			if (answer != (lval | rval)) {
				fprintf(stderr, "result error 0x%x OR 0x%x. Expected=0x%x Encountered 0x%x\n", lval, rval, lval | rval, answer);
			} else if (iAnswer - iOR != pos - iAnswer) {
				fprintf(stderr, "length error 0x%x OR 0x%x. Expected=%d Encountered %d\n", lval, rval, iAnswer - iOR, pos - iAnswer);
			}

			if (0) {
				// display encoded answer
				unsigned k;
				for (k = iOR; k < iAnswer; k++)
					putchar(bit(mem, k) ? '1' : '0');
				putchar('\n');

			}
		}
	}

	return 0;
}
