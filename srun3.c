/*
 * srun3.c
 *
 * @date 2020-07-04 22:55:00
 *
 * Implementation of opcodes for "signed runlength-3" encoding.
 *
 * @date 2020-07-01 02:53:59
 * Previous implementation had the least number of conditionals/jumps. but the bitstream overhead. oo.
 *
 * @date 2020-07-01 14:41:14
 *
 * Note about shrink wrapping leading zeros of the result.
 * Swallowing long lengths of leading zero to emit them all again in case a future "1" appears kills streaming.
 * To make shrink-wrap semi-possible, save the bit position of the last emitted "1".
 * On return, rewind to last "1" and append end-of-sequence marker.
 * The rewind breaks streaming.
 *
 * @date 2020-07-01 21:41:14
 *
 * Test patterns. With `RUNN=3` the longest streak will be 4 bits long.
 * With 3 data paths, using `"3*4+1"` bit values should be sufficient to cover all situations and their harmonics.
 *
 * @date 2020-07-01 14:41:14
 *
 * Drop support of shrink-wrapping results (like in `srun2.c`).
 * - With signed end-of-sequence the code is much more complicated
 * - it rewinds the memory pointer which might break streaming.
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

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// maximum runlength before escaping
#define RUNN 3

// timer tick
int tick = 0;

// what is value of next bit in the sequence
#define nextraw(MEM, POS) ({ unsigned _pos=(POS); ((MEM)[_pos>>3] >> (_pos&7)) & 1; })
// write next bit of the sequence. Old value is undefined and needs explicit set/clear.
#define emitraw(MEM, POS, BIT) ({ unsigned _pos=(POS); if (BIT) (MEM)[_pos>>3] |= 1<<(_pos&7); else (MEM)[_pos>>3] &= ~(1<<(_pos&7)); })

/**
 * Read next input bit, optionally swallowing any escaping markers
 *
 * @param {number} STATE - state of runlength counter
 * @param {number} BIT - read bit. NOTE: do not change, also last read bit
 * @param {number} MEM - bit-addressable memory
 * @param {number} POS - bit-position
 * @return {number} - read data-bit
 */
#define nextcooked(STATE, BIT, MEM, POS) do {			\
	if (STATE) {						\
		if (BIT == nextraw(MEM, POS++)) {		\
			/* same polarity */			\
			STATE <<= 1; /* bump counter */		\
								\
			/* test for escape bit */		\
			if (STATE & (1 << RUNN)) {		\
				/* opposite bit = escape, same bit = EOS */	\
				STATE = nextraw(MEM, POS++) ^ BIT;		\
			}					\
		} else {					\
			/* opposite polarity */			\
			BIT = BIT ^ 1; /* invert */		\
			STATE = 2; /* first bit of sequence */	\
		}						\
	}							\
} while (0)

/**
 * Emit the next output bit, injecting an escape after two consecutive bits.
 *
 * @param {number} STATE - state of runlength counter
 * @param {number} LAST - last emitted bit
 * @param {number} BIT - bit to emit
 * @param {number} MEM - bit-addressable memory
 * @param {number} POS - bit-position
 */
#define emitcooked(STATE, LAST, BIT, MEM, POS) do { \
	/* test if output streak needs to be escaped */		\
	if (STATE & (1 << RUNN)) {				\
		emitraw(MEM, POS++, LAST ^ 1); /* opposite polarity */	\
		STATE = 1; /* reset sequence */			\
	}							\
								\
	/* bit to emit */					\
	emitraw(MEM, POS++, BIT);				\
								\
	STATE = LAST ^ BIT ?					\
		2 /* switching polarity, first bit of sequence */	\
	      :							\
		STATE << 1; /* bump counter */ ;		\
								\
	/* remember last emitted bit */				\
	LAST = BIT;						\
} while (0)

/**
 * Emit end-of-sequence marker
 * Keep emitting leading bits until maximum run-length reached followed by terminator.
 *
 * @param {number} STATE - state of runlength counter
 * @param {number} LAST - last emitted bit
 * @param {number} BIT - polarity
 * @param {number} MEM - bit-addressable memory
 * @param {number} POS - bit-position
 */
#define emitEOS(STATE, LAST, BIT, MEM, POS) do {		\
	/* keep emitting until maximum length reached */	\
	while (!(STATE & (1 << RUNN)) || LAST != BIT)		\
		emitcooked(STATE, LAST, BIT, MEM, POS);		\
								\
	/* finalise end-of-sequence */				\
	emitraw(MEM, POS++, BIT);				\
} while (0)


/**
 * Encode signed value
 *
 * @param {string} pDest - bit-addressable memory (LSB emitted first).
 * @param {number} bitpos - bit-position
 * @param {number} num - signed value
 * @return {number} - length including terminator.
 */
unsigned encode(unsigned char *pDest, unsigned bitpos, int64_t num) {
	unsigned state = 1; // reset sequence
	unsigned last = 0; // initial value not important as sequence is reset

	// as long as there are input bits
	while (num != 0 && num != -1) {
		// extract next LSB from input
		unsigned b = num & 1;
		num >>= 1; // NOTE: arithmetic shift leaves sign-bit untouched

		// inject into output
		emitcooked(state, last, b, pDest, bitpos);
	}

	// end-of-sequence polarity
	num &= 1;

	/*
	 * Buildup leading bits until maximum run-length reached
	 * It is safe to use bit0 of `num` as polarity
	 */
	while (!(state & (1 << RUNN)) || last != num)
		emitcooked(state, last, num, pDest, bitpos);

	// finalise end-of-sequence
	emitraw(pDest, bitpos++, num);

	return bitpos;
}

/**
 * Encode value into runlength-N, return string and length.
 * NOTE: encoded number is unsigned
 *
 * @param {string} pSrc - bit-addressable memory (LSB emitted first).
 * @param {number} bitpos - bit-position
 * @return {int64_t} - The decoded value as variable length structure. For demonstration purpose assuming it will fit in less that 64 bits.
 */
int64_t decode(unsigned char *pSrc, unsigned bitpos) {
	unsigned state = 1; // reset sequence
	unsigned b = 0;
	int64_t num = 0; // number being decoded
	unsigned numlen = 0; // length of `num` in bits

	// The condition is a failsafe as the runN terminator is the end condition
	do {
		nextcooked(state, b, pSrc, bitpos);
		num |= b << numlen;
		numlen++;
	} while (state);

	// fill upper bits of resulting fixed-width number with polarity of end-of-sequence
	num |= -((uint64_t) b << numlen);

	return num;
}

/**
 * @date 2020-07-08 01:15:35
 *
 * Streaming AND
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
	unsigned lstate = 1, lbit = 0;
	unsigned rstate = 1, rbit = 0;
	unsigned estate = 1, ebit, elast = 0;

	do {
		// load next data bit of input pipelines
		nextcooked(lstate, lbit, pL, iL);
		nextcooked(rstate, rbit, pR, iR);

		// operator
		ebit = lbit & rbit;

		// emit operator result
		emitcooked(estate, elast, ebit, pDst, dstpos);
	} while (lstate || rstate);

	// operator on final polarity
	ebit = lbit & rbit;

	// end-of-sequence
	emitEOS(estate, elast, ebit, pDst, dstpos);

	return dstpos;
}

/**
 * @date 2020-07-08 01:16:13
 *
 * Streaming XOR
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
	unsigned lstate = 1, lbit = 0;
	unsigned rstate = 1, rbit = 0;
	unsigned estate = 1, ebit, elast = 0;

	do {
		// load next data bit of input pipelines
		nextcooked(lstate, lbit, pL, iL);
		nextcooked(rstate, rbit, pR, iR);

		// operator
		ebit = lbit ^ rbit;

		// emit operator result
		emitcooked(estate, elast, ebit, pDst, dstpos);
	} while (lstate || rstate);

	// operator on final polarity
	ebit = lbit ^ rbit;

	// end-of-sequence
	emitEOS(estate, elast, ebit, pDst, dstpos);

	return dstpos;
}

/**
 * @date 2020-07-01 22:52:29
 *
 * Streaming OR
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
	unsigned lstate = 1, lbit = 0;
	unsigned rstate = 1, rbit = 0;
	unsigned estate = 1, ebit, elast = 0;

	do {
		// load next data bit of input pipelines
		nextcooked(lstate, lbit, pL, iL);
		nextcooked(rstate, rbit, pR, iR);

		// operator
		ebit = lbit | rbit;

		// emit operator result
		emitcooked(estate, elast, ebit, pDst, dstpos);
	} while (lstate || rstate);

	// operator on final polarity
	ebit = lbit | rbit;

	// end-of-sequence
	emitEOS(estate, elast, ebit, pDst, dstpos);

	return dstpos;
}

/*
 * @date 2020-07-08 00:33:53
 *
 * Signal handler for timer tick
 */
void sigAlarm(int sig) {
	tick++;
	alarm(1);
}

unsigned char mem[512];
unsigned pos;

int main() {
	int64_t num;
	int polarity;

	setlinebuf(stdout);
	signal(SIGALRM, sigAlarm);
	alarm(1);

	/*
	 * Display numbers for visual inspection
	 */
	if (0) {
		for (num = -16; num <= +16; num++) {
			// rewind memory
			pos = 0;

			// encode test value
			pos = encode(mem, pos, num);

			printf("%3d: ", num);

			// display encoded answer
			unsigned k;
			for (k = 0; k < pos; k++)
				putchar(nextraw(mem, k) ? '1' : '0');

			// decode value
			int64_t n;
			n = decode(mem, 0);
			printf(" [%ld]", n);

			putchar('\n');
		}
	}

	/*
	 * Test that `encode/decode` counter each other.
	 */
	for (num = -(1 << 13); num <= +(1 << 13); num++) {
		// rewind memory
		pos = 0;

		// encode test value
		pos = encode(mem, pos, num);

		// decode value
		int64_t n;
		n = decode(mem, 0);

		if (n != num) {
			fprintf(stderr, "encode/decode error. Expected=%ld Encountered=%ld\n", num, n);
			return 1;
		}
	}

	/*
	 * Test that the encoded value has smallest storage.
	 * This is done by filling memory with quasi random bits, decoding+encoding, testing the size of representation being less-equal to the original
	 */
	for (polarity = 0; polarity < 2; polarity++) {
		for (num = 0x0000; num <= 0xffff; num++) {
			// setup memory
			mem[0] = num;
			mem[1] = (num >> 8);
			if (polarity)
				mem[2] = mem[3] = 0xff;
			else
				mem[2] = mem[3] = 0x00;

			// which bit is not part of the leading polarity
			int lengthDecode = 0;
			for (lengthDecode = 31; lengthDecode >= 0; --lengthDecode) {
				if (((mem[lengthDecode / 8] >> (lengthDecode % 8)) & 1) != polarity)
					break;
			}

			// decode value
			int64_t n;
			n = decode(mem, 0);

			// encode it again after presetting all bits of destination
			if (polarity)
				mem[0] = mem[1] = 0xff;
			else
				mem[0] = mem[1] = 0x00;
			encode(mem, 0, n);

			// which bit is not part of the leading polarity
			int lengthEncode = 0;
			for (lengthEncode = 31; lengthEncode >= 0; --lengthEncode) {
				if (((mem[lengthEncode / 8] >> (lengthEncode % 8)) & 1) != polarity)
					break;
			}

			// encode may not be longer than what was decoded
			if (lengthEncode > lengthDecode) {
				fprintf(stderr, "decode/encode length error. mem=%02x.%02x.%02x.%02x num=%ld lengthDecode=%d lengthEncode=%d\n",
					mem[3], mem[2], ((unsigned) num >> 8) & 0xff, (unsigned) num & 0xff, n, lengthDecode, lengthEncode);
				return 1;
			}
		}
	}

	/*
	 * Test all the basic operators
	 */
	unsigned round;
	for (round = 0; round < 10; round++) {
		// display round
		// @formatter:off
		switch (round) {
		case 0: continue; fputs("MUL\n", stdout); break;
		case 1: continue; fputs("DIV\n", stdout); break;
		case 2: continue;  fputs("MOD\n", stdout); break;
		case 3: continue; fputs("ADD\n", stdout); break;
		case 4: continue; fputs("SUB\n", stdout); break;
		case 5: continue; fputs("LSL\n", stdout); break;
		case 6: continue; fputs("LSR\n", stdout); break;
		case 7: fputs("AND\n", stdout); break;
		case 8: fputs("XOR\n", stdout); break;
		case 9: fputs("OR\n", stdout); break;
		}
		// @formatter:on

		int64_t lval, rval;
		int progress = 0;
		for (lval = -(1 << 12); lval <= +(1 << 12); lval++) {
			for (rval = -(1 << 12); rval <= +(1 << 12); rval++) {
				// ticker
				progress++;
				if (tick) {
					fprintf(stderr, "\r\e[K%.2f%%", progress * 100.0 / (4.0 * (1 << 12) * (1 << 12)));
					tick = 0;
				}

				// rewind memory
				pos = 0;

				// encode <left>
				unsigned iL = pos;
				pos = encode(mem, pos, lval);

				// encode <right>
				unsigned iR = pos;
				pos = encode(mem, pos, rval);

				// perform opcode and evaluate native
				unsigned iOpcode = pos;
				int64_t expected = 0;
				switch (round) {
				case 0:
//					pos = MUL(mem, pos, mem, iL, mem, iR);
					expected = lval + rval;
					break;
				case 1:
//					pos = DIV(mem, pos, mem, iL, mem, iR);
					expected = lval + rval;
					break;
				case 2:
//					pos = MOD(mem, pos, mem, iL, mem, iR);
					expected = lval + rval;
					break;
				case 3:
//					pos = ADD(mem, pos, mem, iL, mem, iR);
					expected = lval + rval;
					break;
				case 4:
//					pos = SUB(mem, pos, mem, iL, mem, iR);
					expected = lval - rval;
					break;
				case 5:
					if (rval < 0 || rval > 20)
						continue;
//					pos = LSL(mem, pos, mem, iL, mem, iR);
					expected = lval << rval;
					break;
				case 6:
					if (rval < 0 || rval > 20)
						continue;
//					pos = LSR(mem, pos, mem, iL, mem, iR);
					expected = lval >> rval;
					break;
				case 7:
					pos = AND(mem, pos, mem, iL, mem, iR);
					expected = lval & rval;
					break;
				case 8:
					pos = XOR(mem, pos, mem, iL, mem, iR);
					expected = lval ^ rval;
					break;
				case 9:
					pos = OR(mem, pos, mem, iL, mem, iR);
					expected = lval | rval;
					break;
				};

				// extract
				int64_t answer;
				answer = decode(mem, iOpcode);

				// encode answer to determine length
				unsigned iAnswer = pos;
				pos = encode(mem, pos, answer);

				/*
				 * Compare
				 */
				if (answer != expected) {
					fprintf(stderr, "result error 0x%lx OPCODE 0x%lx. Expected=0x%lx Encountered 0x%lx\n", lval, rval, expected, answer);
					return 1;
				}

				if (0) {
					// display encoded answer
					int k;
					for (k = iOpcode; k < iAnswer; k++)
						putchar(nextraw(mem, k) ? '1' : '0');
					putchar('\n');

				}
			}
		}
		fprintf(stderr, "\r\e[K");
	}

	return 0;
}