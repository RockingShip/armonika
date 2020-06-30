/*
 * generate.c
 *
 * @date 2020-06-30 19:20:55
 *
 * Generate instruction state tables for "unsigned runlength-2" encoding.
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

/*
 * @date 2020-06-30 19:22:19
 *
 * Each state consists of three components:
 * 	1: The state of the Left operand
 * 	2: The state of the Right operand
 * 	3: Number of consecutive  emitted "0"
 *
 * There are 5 data and 3 load operand states:
 * 	- "zero" (data) leading zero's / end of number
 * 	- "1" (data) single bit "1"
 * 	- "01" (data) dounle bit "0" (lsb) and "1" (msb)
 * 	- "00" (data) double zero (after encountering the escape indicator)
 * 	- "0" (data) single bit "0"
 * 	- empty (load) the pre-loader is empty
 * 	- "Z" (load) pre-loader contains a single un-escaped zero
 * 	- "ZZ" (load) pre-loader contains a double un-escaped zero
 *
 * State changes while loading input sequence:
 *
 * 	| state | after reading "0" | after reading "1" |
 * 	|:-----:|:-----------------:|:------------------|
 * 	| empty          Z (load)   |        1  (data)  |
 * 	|   Z            ZZ (load)  |        01 (data)  |
 * 	|   ZZ          zero (data) |        00 (data)  |
 */

// the  7 states
enum { EMPTY, Z, ZZ, D1, D01, D00, D0, ZERO };

// which state has a "1" ready to pop
const unsigned is1[] = { 0, 0, 0, 1, 0, 0, 0, 0 };
// which state has a "0" ready to pop
const unsigned is0[] = { 0, 0, 0, 0, 1, 1, 1, 1 };
// next state after pop
const unsigned pop[] = { EMPTY, Z, ZZ, EMPTY, D1, D0, EMPTY, ZERO };

void genlabel(unsigned L, unsigned R, unsigned N) {
	switch (L) {
	case EMPTY: fputs("L_", stdout); break;
	case Z: fputs("LZ_", stdout); break;
	case ZZ: fputs("LZZ_", stdout); break;
	case D1: fputs("L1_", stdout); break;
	case D01: fputs("L01_", stdout); break;
	case D00: fputs("L00_", stdout); break;
	case D0: fputs("L0_", stdout); break;
	case ZERO: break;
	}

	switch (R) {
	case EMPTY: fputs("R_", stdout); break;
	case Z: fputs("RZ_", stdout); break;
	case ZZ: fputs("RZZ_", stdout); break;
	case D1: fputs("R1_", stdout); break;
	case D01: fputs("R01_", stdout); break;
	case D00: fputs("R00_", stdout); break;
	case D0: fputs("R0_", stdout); break;
	case ZERO: break;
	}

	switch (N) {
	case 0: if (L == ZERO && R == ZERO) fputs("N0_", stdout); break;
	case 1: fputs("N1_", stdout); break;
	case 2: fputs("N2_", stdout); break;
	case 3: fputs("N3_", stdout); break;
	}
}

int genstates() {
	/*
	 * Create state table. A state is either a "load" or an "emit".
	 * Loading takes precedence over data handling.
	 * In case of a draw then Left operand precedes Right operand.
	 */
	for (unsigned stateL=EMPTY; stateL <= ZERO; stateL++) {
		for (unsigned stateR=EMPTY; stateR <= ZERO; stateR++) {
			for (unsigned N=0; N<=2; N++) {
				/*
				 * Generate label
				 */
				genlabel(stateL, stateR, N);
				fputs(":\t", stdout);

				/*
				 * Continue loading until data is achieved
				 */
				if (stateL == EMPTY) {
					fputs("if (bit(pL, iL++)) goto ", stdout);
					genlabel(D1, stateR, N);
					fputs("; else goto ", stdout);
					genlabel(Z, stateR, N);
					fputs(";\n", stdout);
					continue;
				}
				if (stateR == EMPTY) {
					fputs("if (bit(pR, iR++)) goto ", stdout);
					genlabel(stateL, D1, N);
					fputs("; else goto ", stdout);
					genlabel(stateL, Z, N);
					fputs(";\n", stdout);
					continue;
				}
				if (stateL == Z) {
					fputs("if (bit(pL, iL++)) goto ", stdout);
					genlabel(D01, stateR, N);
					fputs("; else goto ", stdout);
					genlabel(ZZ, stateR, N);
					fputs(";\n", stdout);
					continue;
				}
				if (stateR == Z) {
					fputs("if (bit(pR, iR++)) goto ", stdout);
					genlabel(stateL, D01, N);
					fputs("; else goto ", stdout);
					genlabel(stateL, ZZ, N);
					fputs(";\n", stdout);
					continue;
				}
				if (stateL == ZZ) {
					fputs("if (bit(pL, iL++)) goto ", stdout);
					genlabel(D00, stateR, N);
					fputs("; else goto ", stdout);
					genlabel(ZERO, stateR, N);
					fputs(";\n", stdout);
					continue;
				}
				if (stateR == ZZ) {
					fputs("if (bit(pR, iR++)) goto ", stdout);
					genlabel(stateL, D00, N);
					fputs("; else goto ", stdout);
					genlabel(stateL, ZERO, N);
					fputs(";\n", stdout);
					continue;
				}

				/*
				 * Operator logic
				 */

				// emit terminator if still pending
				if (N == 2) {
					if (stateL == ZERO && stateR == ZERO) {
						// end-of-sequence.
						fputs("emit(pDst, dstpos++, 0); return dstpos;\n", stdout);
						continue;
					}
					fputs("emit(pDst, dstpos++, 1); ", stdout);
				}

				if (is1[stateL] || is1[stateR]) {
					// result "1"
					fputs("emit(pDst, dstpos++, 1); goto ", stdout);
					genlabel(pop[stateL], pop[stateR], 0);
					fputs(";\n", stdout);
					continue;
				} else  if (N == 2) {
					// result "0"
					fputs("emit(pDst, dstpos++, 0); goto ", stdout);
					genlabel(pop[stateL], pop[stateR], 1);
					fputs(";\n", stdout);
					continue;
				} else {
					// result "0"
					fputs("emit(pDst, dstpos++, 0); goto ", stdout);
					genlabel(pop[stateL], pop[stateR], N + 1);
					fputs(";\n", stdout);
					continue;
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	fputs("// unsigned OR2(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {\n", stdout);
	genstates();
	fputs("// }\n", stdout);
}
