/*
 * urun2.c
 *
 * @date 2020-06-29 16:25:46
 *
 * Implementation of opcodes for "unsigned runlength-2" encoding.
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

#define bit(MEM,POS) ({ unsigned _pos=(POS); (MEM)[_pos>>3] & (1<<(_pos&7)); })
#define emit(MEM,POS,BIT) ({ unsigned _pos=(POS); (MEM)[_pos>>3] |= (BIT)<<(_pos&7); })

unsigned OR(unsigned char *pDst, unsigned dstpos, unsigned char *pL, unsigned iL, unsigned char *pR, unsigned iR) {

	/*
	 * Determine initial state
	 */
	if (bit(pL, iL++)) goto L1_R_E1; else goto L0_R_E1;

	/*
	 * States that extend L/R without emitting results 
	 */
L_R1_E1: // left empty, right pending "1", last emitted "1"
	if (bit(pL, iL++)) goto L1_R1_E1; else goto L0_R1_E1; // extend L
L_R0_E1: // left empty, right pending "1", last emitted "1"
	if (bit(pL, iL++)) goto L1_R0_E1; else goto L0_R0_E1; // extend L
L1_R_E1: // left pending "1", right empty, last emitted "1"
	if (bit(pR, iR++)) goto L1_R1_E1; else goto L1_R0_E1; // extend R
L0_R_E1: // left pending "0", right empty, last emitted "1"
	if (bit(pR, iR++)) goto L0_R1_E1; else goto L0_R0_E1; // extend R
L0_R1_E1: // left pending "0", right pending "1", last emitted "1"
	if (bit(pL, iL++)) goto L01_R1_E1; else goto L00_R1_E1; // mandatory extend L
L0_R0_E1: // left pending "0", right pending "0", last emitted "1"
	if (bit(pL, iL++)) goto L01_R0_E1; else goto L00_R0_E1; // mandatory extend L
L1_R0_E1: // left pending "1", right pending "0", last emitted "1"
	if (bit(pR, iR++)) goto L1_R01_E1; else goto L1_R00_E1; // mandatory extend R
L01_R0_E1: // left pending "01", right pending "0", last emitted "1"
	if (bit(pR, iR++)) goto L01_R01_E1; else goto L01_R00_E1; // mandatory extend R
L00_R0_E1: // left pending "0", right pending "0", last emitted "1"
	if (bit(pR, iR++)) goto L00_R01_E1; else goto L00_R00_E1; // mandatory extend R

	/*
	 * States free of terminator logic 
	 */
L1_R1_E1: // left pending "1", right pending "1", last emitted "1"
	emit(pDst, dstpos++, 1); // after emitting single bit state is L_R_E1
	if (bit(pL, iL++)) goto L1_R_E1; else goto L0_R_E1; // extend L
L1_R01_E1: // left pending "1", right pending "01", last emitted "1"
	emit(pDst, dstpos++, 1); // after emitting single bit state is L_R1_E1
	if (bit(pL, iL++)) goto L1_R1_E1; else goto L0_R1_E1; // extend L
L01_R1_E1: // left pending "01", right pending "1", last emitted "1"
	emit(pDst, dstpos++, 1); // after emitting single bit state is L1_R_E1
	if (bit(pR, iR++)) goto L1_R1_E1; else goto L1_R0_E1; // extend R
L01_R01_E1: // left pending "01", right pending "01", last emitted "1"
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 1); // after emitting double bit state is L_R_E1
	if (bit(pL, iL++)) goto L1_R_E1; else goto L0_R_E1; // extend L

	/*
	 * Either one or both sides has maximum consecutive zero bits.
	 * Single bit output
	 */
L00_R1_E1: // left pending "00", right pending "1", last emitted "1"
	emit(pDst, dstpos++, 1); // after emitting single bit state is L0_R_E1
	if (!bit(pL, iL++)) goto zero_R_E1; // test if L terminator
	if (bit(pR, iR++)) goto L0_R1_E1; else goto L0_R0_E1; // extend R to balance `bit()`
L1_R00_E1: // left pending "1", right pending "00", last emitted "1"
	emit(pDst, dstpos++, 1); // after emitting single bit state is L_R0_E1
	if (!bit(pR, iR++)) goto L_zero_E1; // test if R terminator
	if (bit(pL, iL++)) goto L1_R0_E1; else goto L0_R0_E1; // extend L to balance `bit()`

	/*
	 * Either one or both sides has maximum consecutive zero bits.
	 * Double bit output
	 */
L00_R01_E1: // left pending "00", right pending "01", last emitted "1"
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 1); // after emitting double bit state is L_R_E1
	if (!bit(pL, iL++)) goto zero_R_E1; // test if L terminator
	if (bit(pR, iR++)) goto L_R1_E1; else goto L_R0_E1; // extend R to balance `bit()`
L01_R00_E1: // left pending "01", right pending "00", last emitted "1"
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 1); // after emitting double bit state is L_R_E1
	if (!bit(pR, iR++)) goto L_zero_E1; // test if R terminator
	if (bit(pL, iL++)) goto L1_R_E1; else goto L0_R_E1; // extend L to balance `bit()`
L00_R00_E1: // left pending "00", right pending "00", last emitted "1"
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 1); // emit double "0" and mandatory escape bit
	if (!bit(pR, iR++)) goto L_zero_E1; // test if R terminator
	if (bit(pL, iL++)) goto L1_R_E1; else goto L0_R_E1; // extend L

	/*
	 * Either side is terminator (infinite leading zeros)
	 */
zero_R_E1: // left zero, right empty, last emitted "1"
	if (bit(pR, iR++)) goto zero_R1_E1; else goto zero_R0_E1; // extend R
zero_R1_E1: // left zero, right pending "1", last emitted "1"
	emit(pDst, dstpos++, 1); // after emitting bit state is zero_R_E1
	if (bit(pR, iR++)) goto zero_R1_E1; else goto zero_R0_E1; // extend R
zero_R0_E1: // left zero, right pending "0", last emitted "1"
	if (bit(pR, iR++)) goto zero_R01_E1; else goto zero_R00_E1; // extend R
zero_R01_E1: // left zero, right pending "01", last emitted "1"
	emit(pDst, dstpos++, 0); // after emitting double bit state is zero_R_E1
	emit(pDst, dstpos++, 1);
	if (bit(pR, iR++)) goto zero_R1_E1; else goto zero_R0_E1; // extend R
zero_R00_E1: // left zero, right pending "00", last emitted "1"
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 1); // emit double "0" and mandatory escape bit
	if (!bit(pR, iR++)) return dstpos; // both terminated, done
	if (bit(pR, iR++)) goto zero_R1_E1; else goto zero_R0_E1; // extend R

L_zero_E1: // left empty, right zero, last emitted "1"
	if (bit(pL, iL++)) goto L1_zero_E1; else goto L0_zero_E1; // extend R
L1_zero_E1: // left pending "1", right zero, last emitted "1"
	emit(pDst, dstpos++, 1); // after emitting bit state is zero_R_E1
	if (bit(pL, iL++)) goto L1_zero_E1; else goto L0_zero_E1; // extend R
L0_zero_E1: // left pending "0", right zero, last emitted "1"
	if (bit(pL, iL++)) goto L01_zero_E1; else goto L00_zero_E1; // extend R
L01_zero_E1: // left pending "01", right zero, last emitted "1"
	emit(pDst, dstpos++, 0); // after emitting double bit state is zero_R_E1
	emit(pDst, dstpos++, 1);
	if (bit(pL, iL++)) goto L1_zero_E1; else goto L0_zero_E1; // extend R
L00_zero_E1: // left pending "00", right zero, last emitted "1"
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 0);
	emit(pDst, dstpos++, 1); // emit double "0" and mandatory escape bit
	if (!bit(pL, iL++)) return dstpos; // both terminated, done
	if (bit(pL, iL++)) goto L1_zero_E1; else goto L0_zero_E1; // extend L
}

unsigned char mem[512];
unsigned pos;

int main() {
	pos = 0;

	/*
	 * L=0b1010
	 */
	unsigned iL = pos;
	emit(mem, pos++, 0);
	printf("pos;%d\n", pos);
	emit(mem, pos++, 1);
	printf("pos;%d\n", pos);
	emit(mem, pos++, 0);
	emit(mem, pos++, 1);
	emit(mem, pos++, 0); // terminator
	emit(mem, pos++, 0);
	emit(mem, pos++, 0);

	/*
	 * R=0b1100
	 */
	unsigned iR = pos;
	emit(mem, pos++, 0);
	emit(mem, pos++, 0);
	emit(mem, pos++, 1); // escape
	emit(mem, pos++, 1);
	emit(mem, pos++, 1);
	emit(mem, pos++, 0); // terminator
	emit(mem, pos++, 0);
	emit(mem, pos++, 0);

	/*
	 * Perform `OR`
	 */
	unsigned newpos = OR(mem, pos, mem, iL, mem, iR);
	printf("pos=%d newpos=%d\n", pos, newpos);

	return 0;
}
