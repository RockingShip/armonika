/*
 * div.cc
 *
 * @date 2020-07-16 23:04:20
 *
 * Tryout for streaming divide.
 * First with constant divisor, then try to automate the manual coefficients
 */

#include <stdint.h>
#include <stdio.h>

/*
 * "double n"
 * n is considered integer and declared double because of the need to get bits behind the decimal point
 */

int64_t div6(int64_t num) {

	double n = num;

	// preamble
	n /= 4;
	n -= n / 2;


	double divisor = 2;
	do {
		divisor *= divisor;
		n = n + n / divisor;
		printf("%.20f\n", n);
	} while (divisor < n);

	printf("%.20f = %ld / 6\n\n", num / 6.0, num);
	return n;
}

int64_t div9(int64_t num) {

	double n = num;

	// preamble
	n /= 8;
	n -= n / 8;

	double divisor = 8;
	do {
		divisor *= divisor;
		n = n + n / divisor;
		printf("%.20f\n", n);
	} while (divisor < n);

	printf("%.20f = %ld / 9\n\n", num / 9.0, num);
	return n;
}

int64_t div15(int64_t num) {

	double n = num;

	// preamble
	n /= 8;
	n -= n / 2;

	double divisor = 4;
	do {
		divisor *= divisor;
		n = n + n / divisor;
		printf("%.20f\n", n);
	} while (divisor < n);

	printf("%.20f = %ld / 15\n\n", num / 15.0, num);
	return n;
}

int64_t div165orig(int64_t num) {

	// 165 = 0b10100101
	double n = num;		printf("%.20f\n", n); // (d<<7) + (d<<5) + (d<<2) + d

	n /= 128;		printf("%.20f\n", n); // d + (d>>2) + (d>>5) + (d>>7)
	n -= n / (1ULL << 2);	printf("%.20f\n", n); // +d +(d>>2) + (d>>5) + (d>>7) - (d>>2) - (d>>4) - (d>>7) - (d>>9) =  +d -(d>>4) +(d>>5) -(d>>9)
	n += n / (1ULL << 4);	printf("%.20f\n", n); // +d -(d>>4) +(d>>5) -(d>>9) +(d>>4) -(d>>8) +(d>>9) -(d>>13) = +d +(d>>5) -(d>>8) -(d>>13)
	n -= n / (1ULL << 5);	printf("%.20f\n", n); // +d +(d>>5) -(d>>8) -(d>>13) -(d>>5) -(d>>10) +(d>>13) +(d>>18) = +d -(d>>8) -(d>>10) +(d>>18)
	n += n / (1ULL << 8);	printf("%.20f\n", n); // +d -(d>>8) -(d>>10) +(d>>18) +(d>>8) -(d>>16) -(d>>18) +(d>>26) = +d -(d>>10) -(d>>16) +(d>>26)
	n += n / (1ULL << 10);	printf("%.20f\n", n); // +d -(d>>10) -(d>>16) +(d>>26) +(d>>10) -(d>>20) -(d>>26) +(d>>36) = +d -(d>>16) -(d>>20) +(d>>36)
	n += n / (1ULL << 16);	printf("%.20f\n", n); // +d -(d>>16) -(d>>20) +(d>>36) +(d>>16) -(d>>32) -(d>>36) +(d>>52) = +d -(d>>20) -(d>>32) +(d>>52)
	n += n / (1ULL << 20);	printf("%.20f\n", n); // +d -(d>>20) -(d>>32) +(d>>52) +(d>>20) -(d>>40) -(d>>52) +(d>>72) = +d -(d>>32) -(d>>40) +(d>>72)
	n += n / (1ULL << 32);	printf("%.20f\n", n); // +d -(d>>32) -(d>>40) +(d>>72) +(d>>32) -(d>>64) -(d>>72) +(d>>104) = +d -(d>>40) -(d>>64) +(d>>104)
	n += n / (1ULL << 40);	printf("%.20f\n", n); // +d -(d>>40) -(d>>64) +(d>>104) +(d>>40) -(d>>80) -(d>>104) +(d>>144) = +d -(d>>64) -(d>>80) +(d>>144)

	printf("%.20f = %ld / 165\n\n", num / 165.0, num);
	return n;
}

void dumpset(uint64_t num, unsigned headpos) {

	putchar('[');
	for (unsigned j=0; j<=headpos; j++) {
		if ( (headpos-j) >= 0 && (headpos-j) < 64)
			if (num&(1ULL<<(headpos-j)))
				printf("%d ", j);
	}
	putchar(']');
	putchar('\n');
}

/*
 * shiftAdd(DST, LEFT, RIGHT, FILL)
 * 
 * DST/LEF/RIGHT are streaming
 * Perform:
 *   if (FILL != 0)
 *      do {
 *   	  "DST = LEFT"
 *   	while (--FILL);  
 *   "DST = LEFT + RIGHT"
 */

/*
 * - divisor sets shift count.
 * - advised instruction `shiftADD` and `shiftOR`
 * 
 */
int64_t div165orig2(int64_t num) {

	/*
	 * Use double to simulate fraction that variable-width storage gives 
	 */
	
	// 165 = 0b10100101
	double n = num;		printf("%.20f\n", n); // (d<<7) + (d<<5) + (d<<2) + d
	uint64_t d;
	
	//  vvv-----add/sub-- arithmetic                            vvv----- logic XOR,SHIFT
								d = 165;
	/*n /= 128;*/		printf("%.20f\n", n); dumpset(d, 7); // d + (d>>2) + (d>>5) + (d>>7)
								d = d ^ (d<<2);
	n -= n / (1ULL << 2);	printf("%.20f\n", n); dumpset(d, 9); // +d +(d>>2) + (d>>5) + (d>>7) - (d>>2) - (d>>4) - (d>>7) - (d>>9) =  +d -(d>>4) +(d>>5) -(d>>9)
								d = d ^ (d<<4); // head is 'd>>4' 
	n += n / (1ULL << 4);	printf("%.20f\n", n); dumpset(d, 13);  // +d -(d>>4) +(d>>5) -(d>>9) +(d>>4) -(d>>8) +(d>>9) -(d>>13) = +d +(d>>5) -(d>>8) -(d>>13)
								d = d ^ (d<<5); // head is 'd>>5' 
	n -= n / (1ULL << 5);	printf("%.20f\n", n); dumpset(d, 18); // +d +(d>>5) -(d>>8) -(d>>13) -(d>>5) -(d>>10) +(d>>13) +(d>>18) = +d -(d>>8) -(d>>10) +(d>>18)
								d = d ^ (d<<8); // head is 'd>>8' 
	n += n / (1ULL << 8);	printf("%.20f\n", n); dumpset(d, 26);  // +d -(d>>8) -(d>>10) +(d>>18) +(d>>8) -(d>>16) -(d>>18) +(d>>26) = +d -(d>>10) -(d>>16) +(d>>26)
								d = d ^ (d<<10); // head is 'd>>10' 
	n += n / (1ULL << 10);	printf("%.20f\n", n); dumpset(d, 36);  // +d -(d>>10) -(d>>16) +(d>>26) +(d>>10) -(d>>20) -(d>>26) +(d>>36) = +d -(d>>16) -(d>>20) +(d>>36)
								d = d ^ (d<<16); // head is 'd>>16' 
	n += n / (1ULL << 16);	printf("%.20f\n", n); dumpset(d, 52);  // +d -(d>>16) -(d>>20) +(d>>36) +(d>>16) -(d>>32) -(d>>36) +(d>>52) = +d -(d>>20) -(d>>32) +(d>>52)
								d = d ^ (d<<20); // head is 'd>>20' 
	n += n / (1ULL << 20);	printf("%.20f\n", n); dumpset(d, 72); // +d -(d>>20) -(d>>32) +(d>>52) +(d>>20) -(d>>40) -(d>>52) +(d>>72) = +d -(d>>32) -(d>>40) +(d>>72)
								d = d ^ (d<<32); // head is 'd>>32' 
	n += n / (1ULL << 32);	printf("%.20f\n", n); dumpset(d, 104); // +d -(d>>32) -(d>>40) +(d>>72) +(d>>32) -(d>>64) -(d>>72) +(d>>104) = +d -(d>>40) -(d>>64) +(d>>104)
								d = d ^ (d<<40); // head is 'd>>40' 
	n += n / (1ULL << 40);	printf("%.20f\n", n); dumpset(d, 144);  // +d -(d>>40) -(d>>64) +(d>>104) +(d>>40) -(d>>80) -(d>>104) +(d>>144) = +d -(d>>64) -(d>>80) +(d>>144)
//								d = d ^ (d<<64); // head is 'd>>40' 
//	n += n / (1ULL << 64);	printf("%.20f\n", n); dumpset(d, 144);  // +d -(d>>64) -(d>>80) +(d>>144) +(d>>64) -(d>>128) -(d>>144) +(d>>208) = +d -(d>>80) -(d>>128) +(d>>208)

	/*
	 * stop when shift is greater than bitlength(num) 
	 */
	
	n /= 128; // perform scaling here as to maximise precision
	
	printf("%.20f = %ld / 165\n\n", num / 165.0, num);
	return n;
}

/*
 * NOTE: 165 is unintentionally a palindrome
 */
int64_t div165(int64_t num) {

	/*
	 * "n -= n >> m" gives rounding errors that could be compensated for with final scaling 
	 */

	// 165 = 0b10100101
	uint64_t n = num;		printf("%ld\n", n); // (d<<7) + (d<<5) + (d<<2) + d
	uint64_t d;

	//  vvv-----add/sub-- arithmetic                            vvv----- XOR,SHIFT logic
	d = 165;
	/*n /= 128; move initial scale to end for higher precision */		
			printf("%ld\n", n >> 7); dumpset(d, 7); // d + (d>>2) + (d>>5) + (d>>7)
								d = d ^ (d<<2); 
	n -= n >> 2;	printf("%ld\n", n >> 7); dumpset(d, 9); // +d +(d>>2) + (d>>5) + (d>>7) - (d>>2) - (d>>4) - (d>>7) - (d>>9) =  +d -(d>>4) +(d>>5) -(d>>9)
								d = d ^ (d<<4); // head is 'd>>4' 
	n += n >> 4;	printf("%ld\n", n >> 7); dumpset(d, 13);  // +d -(d>>4) +(d>>5) -(d>>9) +(d>>4) -(d>>8) +(d>>9) -(d>>13) = +d +(d>>5) -(d>>8) -(d>>13)
								d = d ^ (d<<5); // head is 'd>>5' 
	n -= n >> 5;	printf("%ld\n", n >> 7); dumpset(d, 18); // +d +(d>>5) -(d>>8) -(d>>13) -(d>>5) -(d>>10) +(d>>13) +(d>>18) = +d -(d>>8) -(d>>10) +(d>>18)
								d = d ^ (d<<8); // head is 'd>>8' 
	n += n >> 8;	printf("%ld\n", n >> 7); dumpset(d, 26);  // +d -(d>>8) -(d>>10) +(d>>18) +(d>>8) -(d>>16) -(d>>18) +(d>>26) = +d -(d>>10) -(d>>16) +(d>>26)
								d = d ^ (d<<16); // head is 'd>>16' 
	n += n >> 16;	printf("%ld\n", n >> 7); dumpset(d, 52);  // +d -(d>>16) -(d>>20) +(d>>36) +(d>>16) -(d>>32) -(d>>36) +(d>>52) = +d -(d>>20) -(d>>32) +(d>>52)
								d = d ^ (d<<20); // head is 'd>>20' 
	n += n >> 20;	printf("%ld\n", n >> 7); dumpset(d, 72); // +d -(d>>20) -(d>>32) +(d>>52) +(d>>20) -(d>>40) -(d>>52) +(d>>72) = +d -(d>>32) -(d>>40) +(d>>72)
								d = d ^ (d<<32); // head is 'd>>32' 
	n += n >> 32;	printf("%ld\n", n >> 7); dumpset(d, 104); // +d -(d>>32) -(d>>40) +(d>>72) +(d>>32) -(d>>64) -(d>>72) +(d>>104) = +d -(d>>40) -(d>>64) +(d>>104)
								d = d ^ (d<<40); // head is 'd>>40' 
	n += n >> 40;	printf("%ld\n", n >> 7); dumpset(d, 144);  // +d -(d>>40) -(d>>64) +(d>>104) +(d>>40) -(d>>80) -(d>>104) +(d>>144) = +d -(d>>64) -(d>>80) +(d>>144)

	// note: the runding error due to missing carry is max +/- 9, which stores in 4 bits.
	//       The final scale shifts by 7 giving a safe margin
	
	/*
	 * stop when shift is greater than bitlength(num) 
	 */

	n += (1 << 6); // round
	n >>= 7; // perform scaling here as to maximise precision

	printf("%.20f = %ld / 165\n\n", num / 165.0, num);
	return n;
}

int64_t div3(int64_t num) {

	uint64_t n = num; 	// (d<<1) + d

				printf("%ld\n", n >> 1); // +d + (d>>1)
	n -= n >> 1;	printf("%ld\n", n >> 1); // +d + (d>>1) -(d>>1) - (d>>2) = d -(d>>2)
	n += n >> 2;	printf("%ld\n", n >> 1); // +d -(d>>2) +(d>>2) -(d>>4) = +d -(d>>4)
	n += n >> 4;	printf("%ld\n", n >> 1); // +d -(d>>4) +(d>>4) -(d>>8) = +d -(d>>8) 
	n += n >> 8;	printf("%ld\n", n >> 1); // +d -(d>>8) +(d>>8) -(d>>16) = +d -(d>>16)
	n += n >> 16;	printf("%ld\n", n >> 1); // +d -(d>>16) +(d>>16) -(d>>32) =  +d -(d>>32)
	n += n >> 32;	printf("%ld\n", n >> 1); // 

	n += 1; // round
	n >>= 1; // scale
	
	/*
	 * @date 2020-07-18 00:36:12
	 * NOTE: this is bad, the above has a 3-bit wide rounding error and scaling compensates for only 1 bit.
	 * Solution could be to shift dividend to get more precision
	 * Only an issue for fixed-width as variable-width can keep a longer result for quasi fixed-point (fraction)  
	 */
	
	printf("%.20f = %ld / 3\n\n", num / 3.0, num);
	return n;
}

int main() {
	div6(654321);
	div6(0xcba987654321LL);
	div9(117);
	div9(0xcba987654321LL);
	div15(0xcba987654321LL);
	div165(0xcba987654321LL);

	div3(8);
	div3(9);
	div3(0xcba987654321LL); // rounding error
	
	return 0;
}
