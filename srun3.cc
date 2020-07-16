//#pragma GCC optimize ("O0") // optimize on demand

/*
 * srun3.cc
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

/*
 * @date  2020-07-12 22:58:38
 * 
 * State context/namespace to read sequential memory
 * 
 * @date 2020-07-14 01:29:17
 * 
 * Two shift-registers to determine length of consecutive same polarity.
 * Each with an active bit for positive and negative polarity.
 * The active bit determines the length:
 *   - is repositioned to the head of the queue. (switch)
 *   or
 *   - shifted through the queue. (same)
 * At a certain moment it triggers an arming alert.
 * If the next bit has the same polarity, it indicates end-of-sequence [value of polarity will continue forever].
 * Otherwise mandatory flip polarity.
 * The flip counts as length 1.
 * 
 * @typedef {object} INBIT
 */
struct INBIT {

	/*
	 * @date  2020-07-12 23:00:59
	 * 
	 * State of the input port.
	 * Contains 2 pieces of information:
	 *   - start/stop state. When non-zero the port will read and process bits read from sequential memory.
	 *   - shift register containing a single bit. 
	 * Starting at bit 0, the bit number indicates the number of consecutive same-polarity bits already read.
	 */
	unsigned state;

	/*
	 * @date 2020-07-12 23:05:38
	 * 
	 * Value of decoded bit.
	 * Using `bool` to mark that it conceptually contains a single bit, independent of the implementation.
	 */
	bool bit;

	/*
	 * @date 2020-07-14 22:53:18
	 * 
	 * Memory base address. Addressing is relative to bit 0
	 */
	unsigned char *const pBase;

	/*
	 * @date 2020-07-12 23:06:31
	 * 
	 * Memory is accessed as groups of 8 bit.
	 * Pointer to current byte,
	 */
	unsigned char *pMem;

	/*
	 * @date 2020-07-12 23:08:52
	 * 
	 * Shift register containing bit to indicate active bit within the byte.
	 * On each read the bit is left-rotated one position. 
	 * On wrap, increment memory pointer to next position.
	 */
	unsigned char mask;

	/*
	 * @date 2020-07-12 23:23:42
	 * 
	 * Constructor/Initialise
	 */
	inline INBIT(unsigned char *pBase) : pBase(pBase) {
		this->state = 0; // stop state
		this->pMem = NULL; // current memory location is undefined
		this->mask = 0x01; // Set a single bit
	}

	/*
	 * @date  2020-07-13 22:34:13
	 * 
	 * reset state and set address of first bit of sequential memory
	 */
	inline void start(unsigned pos) {
		this->pMem = this->pBase + (pos >> 3); // in which byte is active bit located
		this->mask = 1 << (pos & 7); // set position of active bit;
		this->state = 1; // start machine and indicate that zero bits have been processed (bit0==1)
		this->bit = 0;
	}

	/*
	 * @date 2020-07-12 23:26:55
	 * 
	 * Read and return next raw bit from memory
	 * 
	 * NOTE: When post-incrementing the active bit, the read value needs to be temporary stored while the increment is in progress.
	 *       Not using a temporary with software requires pre-increment instead of post-increment
	 *       Not using a temporary with hardware the increment can be performed in parallel while the read bit is being processed
	 *       
	 * NOTE: `gcc` on `x86` generates same code complexity with and without the temporary.
	 *       Using post-increment with temporary.  
	 */
	inline unsigned nextraw(void) {

		/*
		 * Extract value of active bit from memory
		 * Note: Two possible implementations:
		 *       `bit = (*pMem & mask) ? 1 : 0;` 
		 *       	is nasty because it (might) generate flow-control instructions.
		 *       	is nice because `mask` is a shift register.
		 *       `bit = (*pMem >> shift);` 
		 *       	is nasty because `shift` is an enumerated value (thus binary encoded).
		 *       	is nice because no flow-control instructions.
		 *       	
		 * For x86 platform the first variant generates `testb/setne` (no flow-control). Using That.
		 */
		unsigned t = (*pMem & mask) ? 1 : 0;

		// shift/rotate the bit in `mask` to mark next active bit
		mask = mask << 1 | mask >> 7;

		// When bit rotates, bump memory pointer
		// do not use 'if (mask & 1) pMem++;` because that generates flow-control instructions
		pMem += (mask & 1);

		// return temporary
		return t;
	}

	/*
	 * @date 2020-07-12 23:57:22
	 * 
	 * Decode next bit from memory
	 */
	inline void nextbit(void) {
		// leave `bit` untouched when in `stop` state
		if (!state)
			return;

		// test for arming of end-of-sequence marker
		if (state & (1 << RUNN)) {
			/*
			 * ARMED, next bit opposite = escape, next bit same = EOS
			 */
			state = nextraw() ^ bit;
			if (!state)
				return; // end-of-sequence. DO NOT read next bit
			bit ^= 1; // mandatory polarity switch
			state <<= 1; // bit is first of run. state should be "1<<1"
		}

		// read next bit from sequential memory and test if there is a polarity switch
		if (bit != nextraw()) {
			/*
			 * Opposite polarity
			 */
			bit ^= 1; // invert polarity
			state = 1; // reset state shift-register
		}

		// Bump the state shift-register indicating the increment of consecutive same-value bits
		state <<= 1;
	}

	/**
	 * @date 2020-07-13 22:06:38
	 * 
	 * Encode value into runlength-N, return string and length.
	 * NOTE: encoded number is unsigned
	 *
	 * @param {string} pSrc - bit-addressable memory (LSB emitted first).
	 * @param {number} bitpos - bit-position
	 * @return {int64_t} - The decoded value as variable length structure. For demonstration purpose assuming it will fit in less that 64 bits.
	 */
	inline int64_t decode(unsigned pos) {
		int64_t num = 0; // fixed-width number being decoded
		unsigned numlen = 0; // length of fixed-width `num` in bits

		// start engine
		start(pos);

		// The condition is a failsafe as the runN terminator is the end condition
		do {
			nextbit();
			num |= (uint64_t) bit << numlen;
			numlen++;
		} while (state);

		// fill upper bits of resulting fixed-width number with polarity of end-of-sequence
		num |= -((uint64_t) bit << numlen);

		return num;
	}

};

/*
 * @date 2020-07-14 01:08:08
 * 
 * State context/namespace to write sequential memory
 *
 * @typedef {object} OUTBIT
 */
struct OUTBIT {

	/*
	 * @date 2020-07-14 01:10:00
	 * 
	 * State of port as a shift register containing a single bit. 
	 * Starting at bit 0, the bit number indicates the number of consecutive same-polarity bits already written.
	 */
	unsigned state;

	/*
	 * @date 2020-07-14 01:10:38
	 * 
	 * Value of last encoded bit written
	 * Using `bool` to mark that it conceptually contains a single bit, independent of the implementation.
	 */
	bool bit;

	/*
	 * @date 2020-07-14 22:53:18
	 * 
	 * Memory base address. Addressing is relative to bit 0
	 */
	unsigned char *const pBase;

	/*
	 * @date 2020-07-12 23:06:31
	 * 
	 * Memory is accessed as groups of 8 bit.
	 * Pointer to current byte,
	 */
	unsigned char *pMem;

	/*
	 * @date 2020-07-14 01:11:24
	 * 
	 * Shift register containing bit to indicate active bit within the byte.
	 * On each write the bit is left-rotated one position. 
	 * On wrap, increment memory pointer to next position.
	 */
	unsigned char mask;

	/*
	 * @date 2020-07-14 01:11:37
	 * 
	 * Constructor/Initialise
	 */
	inline OUTBIT(unsigned char *pBase) : pBase(pBase) {
		state = 0; // stop state
		bit = 0; // last decoded bits
		pMem = NULL; // Memory location is undefined
		mask = 0x01; // Set a single bit
	}

	/*
	 * @date 2020-07-14 01:12:47
	 * 
	 * reset state and set address of first bit of sequential memory
	 */
	inline void start(unsigned pos) {
		this->bit = 0; // not emitted but well known initial value
		this->pMem = pBase + (pos >> 3); // in which byte is active bit located
		this->mask = 1 << (pos & 7); // set position of active bit;
		this->state = 1; // state is nothing previously emitted (bit0 set)
	}

	/*
	 * @date 2020-07-14 21:03:57
	 * 
	 * Return current position of bit/sequential memory
	 */
	unsigned getpos(void) {
		// bit offset = byte offset * 8 + countTrailingZero(mask)
		return (this->pMem - this->pBase) * 8 + __builtin_ctz(mask);
	}

	/*
	 * @date 2020-07-14 20:32:49
	 */
	inline void emitraw(bool b) {
		// set/clear memory bit
		if (b)
			*pMem |= mask;
		else
			*pMem &= ~mask;

		// shift/rotate the bit in `mask` to mark next active bit
		mask = mask << 1 | mask >> 7;

		// When bit rotates, bump memory pointer
		// do not use 'if (mask & 1) pMem++;` because that generates flow-control instructions
		pMem += (mask & 1);
	}

	/*
	 * @date 2020-07-14 20:36:02
	 */
	inline void emitbit(bool b) {
		// if end-of-sequence is armed, emit mandatory escape
		if (state & (1 << RUNN)) {
			bit ^= 1; // flip polarity
			emitraw(bit); // emit polarity switch
			state = 1 << 1; // state = 1 bit emitted
		}

		// emit bit
		emitraw(b);

		// bump state
		state = bit ^ b
			? 1 << 1 // switching polarity, 1 bit emitted
			: state << 1; // shift active bit

		// remember last emitted bit
		bit = b;
	}

	/*
	 * @date 2020-07-15 01:21:22
	 * 
	 * Emit an armed end-of-sequence marker
	 * 
	 * Repeat emitting `value` until maximum run-length reached.
	 * This can generate 0 to RUNN-1 bits.
	 * If `value` is opposite polarity then emit a complete marker.   
	 */
	inline void emitEOSS(bool polarity) {
		while (!(state & (1 << RUNN)) || bit != polarity)
			emitbit(polarity);
	}

	/**
	 * Encode signed value
	 *
	 * @param {string} pDest - bit-addressable memory (LSB emitted first).
	 * @param {number} bitpos - bit-position
	 * @param {number} num - signed value
	 * @return {number} - length including terminator.
	 */
	inline void encode(unsigned pos, int64_t num) {
		// start the engine
		start(pos);

		// as long as there are input bits
		while (num != 0 && num != -1) {
			// inject next LSB bit to output
			emitbit(num & 1);

			// bump binary decoding
			num >>= 1; // NOTE: arithmetic shift leaves sign-bit untouched
		}

		// end-of-sequence polarity
		num &= 1;

		// Buildup leading bits until maximum run-length reached
		emitEOSS(num);

		// finalise end-of-sequence with same-polarity
		emitraw(bit);
	}

};


// timer tick
int tick = 0;

/**
 * @date 2020-07-15 00:52:43
 *
 * Operatore/instructions
 */
struct ALU {

	/*
	 * @date 2020-07-08 18:56:29
	 *
	 * Streaming ADD
	 *
	 * @param out - memory port for result
	 * @param iOut - location of result
	 * @param L - memory port left-hand-side
	 * @param iL - location of left-hand-side
	 * @param R - memory port right-hand-side
	 * @param iR - location right-hand-side
	 */
	inline void ADD(OUTBIT &out, unsigned iOut, INBIT &L, unsigned iL, INBIT &R, unsigned iR) {

		// start engines
		out.start(iOut);
		L.start(iL);
		R.start(iR);

		bool ebit = 0;
		bool carry = 0;

		do {
			// load next data bit of input pipelines
			L.nextbit();
			R.nextbit();

			// operator SUB equals ADD(L,R^!) with inverted carry 
			ebit = carry ^ L.bit ^ R.bit;
			carry = carry ? L.bit | R.bit : L.bit & R.bit;

			// emit operator result
			out.emitbit(ebit);
		} while (L.state || R.state);

		// operator on final polarity
		bool polarity = carry ^ L.bit ^ R.bit;

		/*
		 * @date 2020-07-15 12:11:26
		 * The final carry(`ebit`) needs to be emitted which makes the result 1 bit longer.
		 * Piggyback end-of-sequence-polarity of current streak of same polarity.
		 * Let the caller finalise the end-of-sequence.
		 */
		out.emitEOSS(polarity);

		// finalise end-of-sequence
		out.emitraw(polarity);
	}

	/*
	 * @date 2020-07-08 18:56:29
	 *
	 * Streaming SUB
	 *
	 * NOTE: identical to `ADD` except right-hand-side and carry are inverted
	 *
	 * @param out - memory port for result
	 * @param iOut - location of result
	 * @param L - memory port left-hand-side
	 * @param iL - location of left-hand-side
	 * @param R - memory port right-hand-side
	 * @param iR - location right-hand-side
	 */
	inline void SUB(OUTBIT &out, unsigned iOut, INBIT &L, unsigned iL, INBIT &R, unsigned iR) {

		// start engines
		out.start(iOut);
		L.start(iL);
		R.start(iR);

		bool ebit = 0;
		bool carry = 1;

		do {
			// load next data bit of input pipelines
			L.nextbit();
			R.nextbit();

			// operator SUB equals ADD(L,R^!) with inverted carry 
			ebit = carry ^ L.bit ^ R.bit ^ 1;
			carry = carry ? L.bit | (R.bit ^ 1) : L.bit & (R.bit ^ 1);

			// emit operator result
			out.emitbit(ebit);
		} while (L.state || R.state);

		// operator on final polarity
		bool polarity = (carry ^ 1) ^ L.bit ^ R.bit;

		/*
		 * @date 2020-07-15 12:11:26
		 * The final carry(`ebit`) needs to be emitted which makes the result 1 bit longer.
		 * Piggyback end-of-sequence-polarity of current streak of same polarity.
		 * Let the caller finalise the end-of-sequence.
		 */
		out.emitEOSS(polarity);

		// finalise end-of-sequence
		out.emitraw(polarity);
	}

	/**
	 * @date 2020-07-15 12:36:50
	 *
	 * Logical shift left
	 *
	 * Left-hand-side is streaming
	 * Right-hand-side is enumerated and large values can critically impact operations.
	 * 
	 * @date 2020-07-15 01:42:12
	 * 
	 * The shiftcount is most likely to be less than the length of the sequential memory storage.
	 * right-hand-side could also be a string of which the length determines the shift count. 
	 *
	 * @param out - memory port for result
	 * @param iOut - location of result
	 * @param L - memory port left-hand-side
	 * @param iL - location of left-hand-side
	 * @param R - memory port right-hand-side
	 * @param iR - location right-hand-side
	 */
	inline void LSL(OUTBIT &out, unsigned iOut, INBIT &L, unsigned iL, INBIT &R, unsigned iR) {

		// decode rval
		int rval = R.decode(iR);

		// start engines
		out.start(iOut);
		L.start(iL);

		// emit `rval` number of "0"
		while (rval > 0) {
			out.emitbit(0);
			--rval;
		}

		// Copy lval to output
		do {
			L.nextbit();
			out.emitbit(L.bit);
		} while (L.state);

		// end-of-sequence marker
		out.emitEOSS(L.bit);

		// finalise end-of-sequence
		out.emitraw(L.bit);
	}

	/**
	 * @date 2020-07-15 12:39:51
	 *
	 * Logical shift right
	 *
	 * Left-hand-side is streaming
	 * Right-hand-size is enumerated and large values can critically impact operations.
	 *
	 * @param out - memory port for result
	 * @param iOut - location of result
	 * @param L - memory port left-hand-side
	 * @param iL - location of left-hand-side
	 * @param R - memory port right-hand-side
	 * @param iR - location right-hand-side
	 */
	inline void LSR(OUTBIT &out, unsigned iOut, INBIT &L, unsigned iL, INBIT &R, unsigned iR) {

		// decode rval
		int rval = R.decode(iR);

		// start engines
		out.start(iOut);
		L.start(iL);

		// Copy lval to output skipping first `rval` bits
		do {
			L.nextbit();
			if (--rval < 0)
				out.emitbit(L.bit);
		} while (L.state);

		// end-of-sequence marker
		out.emitEOSS(L.bit);

		// finalise end-of-sequence
		out.emitraw(L.bit);
	}

	/**
	 * @date 2020-07-15 12:22:27
	 *
	 * Streaming AND
	 *
	 * @param out - memory port for result
	 * @param iOut - location of result
	 * @param L - memory port left-hand-side
	 * @param iL - location of left-hand-side
	 * @param R - memory port right-hand-side
	 * @param iR - location right-hand-side
	 */
	inline void AND(OUTBIT &out, unsigned iOut, INBIT &L, unsigned iL, INBIT &R, unsigned iR) {

		// start engines
		out.start(iOut);
		L.start(iL);
		R.start(iR);

		do {
			// load next data bit of input pipelines
			L.nextbit();
			R.nextbit();

			// operator 
			bool ebit = L.bit & R.bit;

			// emit operator result
			out.emitbit(ebit);
		} while (L.state || R.state);

		// final polarity
		bool polarity = L.bit & R.bit;

		// end-of-sequence marker
		out.emitEOSS(polarity);

		// finalise end-of-sequence
		out.emitraw(polarity);
	}

	/**
	 * @date 2020-07-15 12:31:06
	 *
	 * Streaming XOR
	 *
	 * @param out - memory port for result
	 * @param iOut - location of result
	 * @param L - memory port left-hand-side
	 * @param iL - location of left-hand-side
	 * @param R - memory port right-hand-side
	 * @param iR - location right-hand-side
	 */
	inline void XOR(OUTBIT &out, unsigned iOut, INBIT &L, unsigned iL, INBIT &R, unsigned iR) {

		// start engines
		out.start(iOut);
		L.start(iL);
		R.start(iR);

		do {
			// load next data bit of input pipelines
			L.nextbit();
			R.nextbit();

			// operator 
			bool ebit = L.bit ^ R.bit;

			// emit operator result
			out.emitbit(ebit);
		} while (L.state || R.state);

		// final polarity
		bool polarity = L.bit ^ R.bit;

		// end-of-sequence marker
		out.emitEOSS(polarity);

		// finalise end-of-sequence
		out.emitraw(polarity);
	}

	/**
	 * @date 2020-07-15 12:32:23
	 *
	 * Streaming OR
	 *
	 * @param out - memory port for result
	 * @param iOut - location of result
	 * @param L - memory port left-hand-side
	 * @param iL - location of left-hand-side
	 * @param R - memory port right-hand-side
	 * @param iR - location right-hand-side
	 */
	inline void OR(OUTBIT &out, unsigned iOut, INBIT &L, unsigned iL, INBIT &R, unsigned iR) {

		// start engines
		out.start(iOut);
		L.start(iL);
		R.start(iR);

		do {
			// load next data bit of input pipelines
			L.nextbit();
			R.nextbit();

			// operator 
			bool ebit = L.bit | R.bit;

			// emit operator result
			out.emitbit(ebit);
		} while (L.state || R.state);

		// final polarity
		bool polarity = L.bit | R.bit;

		// end-of-sequence marker
		out.emitEOSS(polarity);

		// finalise end-of-sequence
		out.emitraw(polarity);
	}

};

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
	setlinebuf(stdout);
	signal(SIGALRM, sigAlarm);
	alarm(1);

	INBIT ib(mem), L(mem), R(mem);
	OUTBIT ob(mem);

	/*
	 * Display numbers for visual inspection
	 */
	if (0) {
		for (int64_t num = -128; num <= +128; num++) {
			// rewind memory
			pos = 0;

			// encode test value
			ob.encode(pos, num);
			pos = ob.getpos();

			printf("%3d: ", (int) num);

			// display encoded answer
			ib.start(0);
			unsigned k;
			for (k = 0; k < pos; k++)
				putchar(ib.nextraw() ? '1' : '0');

			// decode value
			int64_t n;
			n = ib.decode(0);
			printf(" [%ld]", n);

			putchar('\n');
		}
	}

	/*
	 * Test that `encode/decode` counter each other.
	 */
	for (int64_t num = +(1 << 13); num <= +(1 << 13); num++) {
		// rewind memory
		pos = 0;

		// encode test value
		ob.encode(pos, num);
		pos = ob.getpos();

		// decode value
		int64_t n;
		n = ib.decode(0);

		if (n != num) {
			fprintf(stderr, "encode/decode error. Expected=%ld Encountered=%ld\n", num, n);
			return 1;
		}
	}

	/*
	 * Test that the encoded value has smallest storage.
	 * This is done by filling memory with quasi random bits, decoding+encoding, testing the size of representation being less-equal to the original
	 */
	for (int polarity = 0; polarity < 2; polarity++) {
		for (int64_t num = 0x0000; num <= 0xffff; num++) {
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
			n = ib.decode(0);

			// encode it again after presetting all bits of destination
			if (polarity)
				mem[0] = mem[1] = 0xff;
			else
				mem[0] = mem[1] = 0x00;
			ob.encode(0, n);

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
	struct ALU alu;

	for (unsigned round = 0; round < 10; round++) {
		// display round
		// @formatter:off
		switch (round) {
		case 0: continue; fputs("MUL\n", stdout); break;
		case 1: continue; fputs("DIV\n", stdout); break;
		case 2: continue; fputs("MOD\n", stdout); break;
		case 3: fputs("ADD\n", stdout); break;
		case 4: fputs("SUB\n", stdout); break;
		case 5: fputs("LSL\n", stdout); break;
		case 6: fputs("LSR\n", stdout); break;
		case 7: fputs("AND\n", stdout); break;
		case 8: fputs("XOR\n", stdout); break;
		case 9: fputs("OR\n", stdout); break;
		}
		// @formatter:on

		int progress = 0;
		for (int64_t lval = -(1 << 12); lval <= +(1 << 12); lval++) {
			for (int64_t rval = -(1 << 12); rval <= +(1 << 12); rval++) {
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
				ob.encode(pos, lval);
				pos = ob.getpos();

				// encode <right>
				unsigned iR = pos;
				ob.encode(pos, rval);
				pos = ob.getpos();

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
					alu.ADD(ob, pos, L, iL, R, iR);
					pos = ob.getpos();
					expected = lval + rval;
					break;
				case 4:
					alu.SUB(ob, pos, L, iL, R, iR);
					pos = ob.getpos();
					expected = lval - rval;
					break;
				case 5:
					if (rval < 0 || rval > 20)
						continue; // limit range
					alu.LSL(ob, pos, L, iL, R, iR);
					pos = ob.getpos();
					expected = lval << rval;
					break;
				case 6:
					if (rval < 0 || rval > 20)
						continue; // limit range
					alu.LSR(ob, pos, L, iL, R, iR);
					pos = ob.getpos();
					expected = lval >> rval;
					break;
				case 7:
					alu.AND(ob, pos, L, iL, R, iR);
					pos = ob.getpos();
					expected = lval & rval;
					break;
				case 8:
					alu.XOR(ob, pos, L, iL, R, iR);
					pos = ob.getpos();
					expected = lval ^ rval;
					break;
				case 9:
					alu.OR(ob, pos, L, iL, R, iR);
					pos = ob.getpos();
					expected = lval | rval;
					break;
				};

				// extract
				int64_t answer;
				answer = ib.decode(iOpcode);

				/*
				 * Compare
				 */
				if (answer != expected) {
					fprintf(stderr, "result error 0x%lx OPCODE 0x%lx. Expected=0x%lx Encountered 0x%lx\n", lval, rval, expected, answer);
					return 1;
				}

				if (0) {
					// encode answer to determine length
					unsigned iAnswer = pos;
					ob.encode(pos, answer);
					pos = ob.getpos();

					// display encoded answer
					for (unsigned k = iOpcode; k < iAnswer; k++) {
						ib.start(k);
						ib.nextbit();
						putchar(ib.bit ? '1' : '0');
					}
					putchar('\n');

				}
			}
		}
		fprintf(stderr, "\r\e[K");
	}

	return 0;
}
