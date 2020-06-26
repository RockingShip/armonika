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
unsigned encode(char *pDest, uint64_t num, int N) {
	unsigned count = 0; // current runlength (number of consecutive bits of same polarity)
	unsigned last = 0; // last bit that was output (value has no meaning when count=0)
	unsigned length = 0; // length of encoded result

	// as long as there are input bits
	while (num) {
		// extract next LSB from input
		unsigned bit = num & 1;
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

	// inject terminator
	while (N > 0) {
		*pDest++ = '0';
		length++;
		--N;
	}

	// string terminator
	*pDest = 0;

	return length;
}

int main(int argc, char *argv[]) {
	char dest[128]; // storage for encoded string
	int counts[128]; // frequency count
	int N; // runlength
	int k;

	for (k=0; k<20; k++)
		encode(dest,k,3), puts(dest);
	return 0;

	for (N = runlengthMin; N <= runlengthMax; ++N) {
		// clear counts
		for (k = 0; k < 128; ++k)
			counts[k] = 0;

		// encode numbers and count length
		for (k = 0; k < numMax; ++k)
			counts[encode(dest, k, N)] += 1;

		// display frequency count
		printf("N=%d\n", N);
		for (k = 0; k < 128; ++k)
			if (counts[k])
				printf("%2d: %d\n", k, counts[k]);
	}

	return 0;
}

