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

int64_t div165(int64_t num) {

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

int main() {
	div6(654321);
	div6(0xcba987654321LL);
	div9(117);
	div9(0xcba987654321LL);
	div15(0xcba987654321LL);
	div165(0xcba987654321LL);
	return 0;
}
