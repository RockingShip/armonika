/*
 * frequency.c
 *
 * @date 2020-06-26 00:06:58
 *
 * Encode numbers with different N and frequency count the resulting lengths.
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

enum {
	runlengthMin = 2, // lowbound for runlength
	runlengthMax = 5, // highbound for runlength
	numMax = 65536, // highest+1 number to encode
};

/**
 * Encode value into runlength-N, return string and length.
 * NOTE: encoded number is unsigned
 *
 * @param {string} pDest - Encoded stream as string (LSB first).
 * @param {number} num - number to encode.
 * @param {number} N - runlength
 * @return {number} - length including terminator.
 */
unsigned encode(char *pDest, int64_t num, int N) {
	unsigned count = 0; // current runlength (number of consecutive bits of same polarity)
	unsigned last = 0; // last bit that was output (value has no meaning when count=0)
	unsigned length = 0; // length of encoded result

	// as long as there are input bits
	while (num && num != -1) {
		// extract next LSB from input
		unsigned bit = num & 1;
		// NOTE: right shift of signed negative number will keep MSB set to "1". This will continue until "-1" where it stays 'unchanged'.
		num >>= 1;

		// inject into output
		*pDest++ = '0' + bit;
		length++;

		// update runlength
		if (last != bit) {
			// polarity changed
			last = bit;
			count = 1;
		} else if (++count == N) {
			// runlength limit reached, inject opposite polarity
			*pDest++ = '1' - bit;
			length++;
			last = 1 - bit;
			count = 1;
		}
	}

	// get polarity number (either all bits "0"" or all bits "1"
	num &= 1;

	// reset run length if last bit output has different polarity than terminator
	if (last != num)
		count = 0;

	// append terminator
	while (count <= N) {
		*pDest++ = '0' + num;
		length++;
		++count;
	}

	// string terminator
	*pDest = 0;

	return length;
}

/**
 * Encode value into runlength-N, return string and length.
 * NOTE: encoded number is unsigned
 *
 * @param {string} pSource - Start of sequence as string (LSB first).
 * @param {number} N - runlength
 * @return {int64_t} - The decoded value as variable length structurele. for demonstration purpose assuming it will fit in less that 64 bits.
 */
int64_t decode(char *pSrc, int N) {
	unsigned count = 0; // current runlength (number of consecutive bits of same polarity)
	unsigned last = 0; // last bit that was output (value has no meaning when count=0)
	unsigned length = 0; // length of encoded result
	int64_t num = 0; // number being decoded

	// The condition is a failsafe as the runN terminator is the end condition
	while (*pSrc) {
		// extract next LSB from input
		uint64_t bit = *pSrc++ - '0';

		// inject into output
		num |= bit << length;
		length++;

		// update runlength
		if (last != bit) {
			// polarity changed
			last = bit;
			count = 1;
		} else if (++count == N) {
			// runlength reached, get next bit
			bit = *pSrc++ - '0';
			if (bit == last)
				break; // N+1 consecutive bits of same polarity is terminator

			// swap polatity
			last = 1 - last;
			count = 1;
		}
	}

	// fill upper bits of fixed width number with polarity of terminator
	num |= -((uint64_t) last << length);

	return num;
}

int main(int argc, char *argv[]) {
	char dest[128]; // storage for encoded string
	int counts[128]; // frequency count
	int N; // runlength
	int k;

	/*
	 * Frequency count selected numbers.
	 */
	for (N = runlengthMin; N <= runlengthMax; ++N) {
		// clear counts
		for (k = 0; k < 128; ++k)
			counts[k] = 0;

		// encode numbers and count length
		// NOTE: Both negative as positive numbers
		for (k = -numMax; k < numMax; ++k) {
			// encode number
			unsigned length = encode(dest, k, N);
			// test that it properly decodes
			int64_t decoded = decode(dest, N);
			// test identical
			if (k != decoded) {
				fprintf(stderr, "Selftest failure. Expected %x, encountered %lx\n", k, decoded);
				return 1;
			}

			// frequency count length
			++counts[length];
		}

		// display frequency count
		printf("N=%d\n", N);
		for (k = 0; k < 128; ++k)
			if (counts[k])
				printf("%2d: %d\n", k, counts[k]);
	}

	return 0;
}

