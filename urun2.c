/*
 * urun2.c
 *
 * @date 2020-06-29 16:25:46
 *
 * Implementation of opcodes for "unsigned runlength-2" encoding.
 *
 * @date 2020-07-01 02:53:59
 * Previous implementation had the least number of conditionals/jumps. but the bitstream overhead. oo.
 *
 * @date 2020-07-01 14:41:14
 * Note about shrink wrapping leading zeros of the result.
 * Swallowing long lengths of leading zero to emit them all again in case a future "1" appears kills streaming.
 * To make shrink-wrap semi-possible, save the bit position of the last emitted "1".
 * On return, rewind to last "1" and append end-of-sequence marker.
 * The rewind breaks streaming.
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

/**
 * @date 2020-07-01 22:52:29
 *
 * Logical shift left
 *
 * @param pDst 	 bit-memory base of result
 * @param dstpos - bit position of result
 * @param pL - bit-memory base of left-hand-side
 * @param iL - bit position of left-hand-side
 * @param pR - bit-memory base of right-hand-side
 * @param iR - bit position of right-hand-side
 * @return dstpos + encoded-length
 */
unsigned LSL(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {

	unsigned lstate = 1, lbit;
	unsigned rstate = 1, rbit;
	unsigned estate = 1, ebit;
	unsigned last1 = dstpos;

	/*
	 * LSL is not fully async streaming. It needs to decode rval first to determine the shift count. Luckily the range of rval is usually small
	 */

	unsigned rval = 0; // number being decoded
	unsigned mask = 1; // active bit in rval

	// fast decode
	while (rstate) {
		rbit = bit(pR, iR++) ? 1 : 0;
		rstate = rbit ? 1 : rstate << 1;

		if (rbit)
			rval |= mask;
		mask <<= 1;

		if (rstate == 4)
			rstate = bit(pR, iR++) ? 1 : 0; // either end-of-sequence on "0", knowing that two "0" already have neen emitted. the third is also a terminator.
	}

	/*
	 * emit `rval number of "0"
	 */
	while (rval > 0) {
		/*
		 * Escape current streak when two consecutivee "0" have already been emitted.
		 * Do this before emitting data because if data were "0" it could collapse with the end-of-sequence
		 */
		if (estate == 4) {
			emit(pDst, dstpos++, 1);
			estate = 1;
		}

		/*
		 * Emit "0"
		 */
		emit(pDst, dstpos++, 0);
		estate <<= 1;

		--rval;
	}

	/*
	 * Copy lval to output
	 */
	while (lstate) {
		/*
		 * Load next lval bit
		 */
		lbit = bit(pL, iL++) ? 1 : 0;
		lstate = lbit ? 1 : lstate << 1;

		if (lstate == 4)
			lstate = bit(pL, iL++) ? 1 : 0; // either end-of-sequence on "0", knowing that two "0" already have neen emitted. the third is also a terminator.

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
		ebit = lbit;
		emit(pDst, dstpos++, ebit);
		// update position last data "1" for shrink-wrapping
		if (ebit)
			last1 = dstpos;

		/*
		 * Emitting "1" resets the runlength counter
		 */
		estate = ebit ? 1 : estate << 1;

	}

	/*
	 * In case shrink-wrapping enqabled, rewind to the last data-"1" anf append end-of-sequence marker
	 */
	if (1) {
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		return last1;
	}

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
 * @date 2020-07-01 22:52:29
 *
 * Logical shift right
 *
 * @param pDst 	 bit-memory base of result
 * @param dstpos - bit position of result
 * @param pL - bit-memory base of left-hand-side
 * @param iL - bit position of left-hand-side
 * @param pR - bit-memory base of right-hand-side
 * @param iR - bit position of right-hand-side
 * @return dstpos + encoded-length
 */
unsigned LSR(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {
	unsigned lstate = 1, lbit;
	unsigned rstate = 1, rbit;
	unsigned estate = 1, ebit;
	unsigned last1 = dstpos;

	/*
	 * LSR is not fully async streaming. It needs to decode rval first to determine the shift count. Lockily the range of rval is usually small
	 */

	unsigned rval = 0; // number being decoded
	unsigned mask = 1; // active bit in rval

	// fast decode
	while (rstate) {
		rbit = bit(pR, iR++) ? 1 : 0;
		rstate = rbit ? 1 : rstate << 1;

		if (rbit)
			rval |= mask;
		mask <<= 1;

		if (rstate == 4)
			rstate = bit(pR, iR++) ? 1 : 0; // either end-of-sequence on "0", knowing that two "0" already have neen emitted. the third is also a terminator.
	}

	/*
	 * Swallow `rval` number of lval bits
	 */
	while (lstate && rval) {
		lbit = bit(pL, iL++) ? 1 : 0;
		lstate = lbit ? 1 : lstate << 1;

		if (lstate == 4)
			lstate = bit(pL, iL++) ? 1 : 0; // either end-of-sequence on "0", knowing that two "0" already have neen emitted. the third is also a terminator.

		--rval;
	}

	/*
	 * copy remainder of lval to output
	 */
	while (lstate) {
		/*
		 * Load next lval bit
		 */
		lbit = bit(pL, iL++) ? 1 : 0;
		lstate = lbit ? 1 : lstate << 1;

		if (lstate == 4)
			lstate = bit(pL, iL++) ? 1 : 0; // either end-of-sequence on "0", knowing that two "0" already have neen emitted. the third is also a terminator.

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
		ebit = lbit;
		emit(pDst, dstpos++, ebit);
		// update position last data "1" for shrink-wrapping
		if (ebit)
			last1 = dstpos;

		/*
		 * Emitting "1" resets the runlength counter
		 */
		estate = ebit ? 1 : estate << 1;

	}

	/*
	 * In case shrink-wrapping enqabled, rewind to the last data-"1" anf append end-of-sequence marker
	 */
	if (1) {
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		return last1;
	}

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
 * @date 2020-07-01 22:52:29
 *
 * Logical AND
 *
 * @param pDst 	 bit-memory base of result
 * @param dstpos - bit position of result
 * @param pL - bit-memory base of left-hand-side
 * @param iL - bit position of left-hand-side
 * @param pR - bit-memory base of right-hand-side
 * @param iR - bit position of right-hand-side
 * @return dstpos + encoded-length
 */
unsigned AND(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {

	// three pipelines, two for left/right operands one for the result.
	// two consecutive "0" is the trigger. Either a "0" to end the sequence or "1" to escape and continue.
	//
	unsigned lstate = 1, lbit;
	unsigned rstate = 1, rbit;
	unsigned estate = 1, ebit;
	unsigned last1 = dstpos;

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
		ebit = lbit & rbit;
		emit(pDst, dstpos++, ebit);
		// update position last data "1" for shrink-wrapping
		if (ebit)
			last1 = dstpos;

		/*
		 * Emitting "1" resets the runlength counter
		 */
		estate = ebit ? 1 : estate << 1;

	} while (lstate | rstate);
	// lstate | rstate requires a merge ("|") and the vector is tested for zero.
	// lstate || rstate requires two tests.

	/*
	 * In case shrink-wrapping enqabled, rewind to the last data-"1" anf append end-of-sequence marker
	 */
	if (1) {
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		return last1;
	}

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
 * @date 2020-07-01 22:52:29
 *
 * Logical XOR
 *
 * @param pDst 	 bit-memory base of result
 * @param dstpos - bit position of result
 * @param pL - bit-memory base of left-hand-side
 * @param iL - bit position of left-hand-side
 * @param pR - bit-memory base of right-hand-side
 * @param iR - bit position of right-hand-side
 * @return dstpos + encoded-length
 */
 unsigned XOR(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {

	// three pipelines, two for left/right operands one for the result.
	// two consecutive "0" is the trigger. Either a "0" to end the sequence or "1" to escape and continue.
	//
	unsigned lstate = 1, lbit;
	unsigned rstate = 1, rbit;
	unsigned estate = 1, ebit;
	unsigned last1 = dstpos;

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
		ebit = lbit ^ rbit;
		emit(pDst, dstpos++, ebit);
		// update position last data "1" for shrink-wrapping
		if (ebit)
			last1 = dstpos;

		/*
		 * Emitting "1" resets the runlength counter
		 */
		estate = ebit ? 1 : estate << 1;

	} while (lstate | rstate);
	// lstate | rstate requires a merge ("|") and the vector is tested for zero.
	// lstate || rstate requires two tests.

	/*
	 * In case shrink-wrapping enqabled, rewind to the last data-"1" anf append end-of-sequence marker
	 */
	if (1) {
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		return last1;
	}

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
 * @date 2020-07-01 22:52:29
 *
 * Logical OR
 *
 * @param pDst 	 bit-memory base of result
 * @param dstpos - bit position of result
 * @param pL - bit-memory base of left-hand-side
 * @param iL - bit position of left-hand-side
 * @param pR - bit-memory base of right-hand-side
 * @param iR - bit position of right-hand-side
 * @return dstpos + encoded-length
 */
 unsigned OR(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {

	// three pipelines, two for left/right operands one for the result.
	// two consecutive "0" is the trigger. Either a "0" to end the sequence or "1" to escape and continue.
	//
	unsigned lstate = 1, lbit;
	unsigned rstate = 1, rbit;
	unsigned estate = 1, ebit;
	unsigned last1 = dstpos;

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

		// update position last data "1" for shrink-wrapping
		if (ebit)
			last1 = dstpos;

		/*
		 * Emitting "1" resets the runlength counter
		 */
		estate = ebit ? 1 : estate << 1;

	} while (lstate | rstate);
	// lstate | rstate requires a merge ("|") and the vector is tested for zero.
	// lstate || rstate requires two tests.

	/*
	 * In case shrink-wrapping enqabled, rewind to the last data-"1" anf append end-of-sequence marker
	 */
	if (1) {
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		emit(pDst, last1++, 0);
		return last1;
	}

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
	setlinebuf(stdout);

	/*
	 * Test the function by trying all 12-bit possibilities per variable for "<left> OR <right>
	 */
	unsigned round, lval, rval;
	for (round=0; round<5; round++) {
		// display round
		// @formatter:off
		switch (round) {
		case 0: fputs("LSL\n", stdout); break;
		case 1: fputs("LSR\n", stdout); break;
		case 2: fputs("AND\n", stdout); break;
		case 3: fputs("XOR\n", stdout); break;
		case 4: fputs("OR\n", stdout); break;
		}
		// @formatter:on

		for (lval = 0; lval < (1 << 12); lval++) {
			for (rval = 0; rval < (1 << 12); rval++) {

				// rewind memort
				pos = 0;

				// encode <left>
				unsigned iL = pos;
				pos = encode(mem, pos, lval, 2);

				// encode <right>
				unsigned iR = pos;
				pos = encode(mem, pos, rval, 2);

				// perform opcode and evaluate native
				unsigned iOpcode = pos;
				unsigned expected = 0;
				switch (round) {
				case 0:
					if (rval > 20)
						continue;
					pos = LSL(mem, pos, mem, iL, mem, iR);
					expected = lval << rval;
					break;
				case 1:
					if (rval > 20)
						continue;
					pos = LSR(mem, pos, mem, iL, mem, iR);
					expected = lval >> rval;
					break;
				case 2:
					pos = AND(mem, pos, mem, iL, mem, iR);
					expected = lval & rval;
					break;
				case 3:
					pos = XOR(mem, pos, mem, iL, mem, iR);
					expected = lval ^ rval;
					break;
				case 4:
					pos = OR(mem, pos, mem, iL, mem, iR);
					expected = lval | rval;
					break;
				};

				// extract
				uint64_t answer;
				answer = decode(mem, iOpcode, 2);

				// encode answer to determine length
				unsigned iAnswer = pos;
				pos = encode(mem, pos, answer, 2);

				/*
				 * Compare
				 */
				if (answer != expected) {
					fprintf(stderr, "result error 0x%x OPCODE 0x%x. Expected=0x%x Encountered 0x%x\n", lval, rval, expected, answer);
				} else if (iAnswer - iOpcode != pos - iAnswer) {
					fprintf(stderr, "length error 0x%x OPCODE 0x%x. Expected=%d Encountered %d\n", lval, rval, iAnswer - iOpcode, pos - iAnswer);
				}

				if (0) {
					// display encoded answer
					unsigned k;
					for (k = iOpcode; k < iAnswer; k++)
						putchar(bit(mem, k) ? '1' : '0');
					putchar('\n');

				}
			}
		}
	}

	return 0;
}
