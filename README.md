# Armonika

Encoding/decoding/handling of variable length numbers in bit addressable memory.

Addressing data in memory requires two components: location (address) and size (bit count).

With fixed width memory (multiple of power-of-2 bytes) the maximum storage size is determined at compile-time
whereas with variable-width memory the unrestricted storage size is determined at run-time.

This project is an experimental environment for storing data in bit-addressable memory.
It is intended to bridge the worlds of the `xtools` and `untangle` projects.

The proposed encoding has these features:
 - Streaming.
 - No maximum size.
 - Simple and fast encoding/decoding.
 - Simple and fast length detection.
 - Simple functional implementation of bitwise and integer arithmetic operators.
 - Simple hardware implementation.

# Encoding/decoding scheme

```
    After n consecutive "0", force emit (on encoding) a "1" which is (force ignored) during decoding.
    n+1 consecutive "0" indicitates leading-zeros/end-of-number.
```

`untangle` in unfamiliar with the concept "two-complement" whereas `xtools` assumed its presence.
Negative Two-complement numbers have an infinite number of leading "1".
Extend the encoding that:

```
    After n consecutive "1", force emit (on encoding) a "0" which is (force ignored) during decoding.
    n+1 consecutive "1" indicitates leading-ones/end-of-number.
```

# Examples

The following tables demonstrates the encoding with nthe numbers 0-19.

The binary input is read right-to-left, the string output is read left-to-right. 

| Input binary | Output N=2 | output N=3 |
|------:|:-----------|:-----------|
| <---- | ------->   | ------->
| 00000 | 000        | 0000      
| 00001 | 1000       | 10000     
| 00010 | 01000      | 010000    
| 00011 | 11000      | 110000    
| 00100 | 0011000    | 0010000   
| 00101 | 101000     | 1010000   
| 00110 | 011000     | 0110000   
| 00111 | 1101000    | 1110000   
| 01000 | 00101000   | 000110000 
| 01001 | 10011000   | 10010000  
| 01010 | 0101000    | 01010000  
| 01011 | 110011000  | 11010000  
| 01100 | 001101000  | 00110000  
| 01101 | 1011000    | 10110000  
| 01110 | 01101000   | 01110000  
| 01111 | 11011000   | 111010000 
| 10000 | 0010011000 | 0001010000
| 10001 | 100101000  | 1000110000
| 10010 | 010011000  | 010010000 
| 10011 | 1100101000 | 110010000 
   
The first number that does not fit in 32 bits:
 - for N=2. 43691 (0x0000aaab) which is encoded as "110011001100110011001100110011000".
 - for N=3. 629144 (0x00099998) which is encoded as "110011001100110011001100110011000".
 - for N=4. 2098063 (0x0020038f) which is encoded as "111100001111000010000100001100000".
 - for N=5. 2220512 (0x0021e1e0) which is encoded as "000001111100000111110000011000000".

# Frequency count

Lowest `N` is simplest implementation, higher `N` is greater storage efficiency.
To aid in making a choice the following table contains frequency counts for encoded lengths with different settings of `N`.
Counted are all the values in range 0-65535.

| length | N = 2 | N = 3 | N = 4 | N = 5 | notes |
|-------:|------:|------:|------:|------:|:------|
|  3 |     1 |       |       |       |
|  4 |     1 |     1 |       |       |
|  5 |     2 |     1 |     1 |       |
|  6 |     2 |     2 |     1 |     1 |
|  7 |     4 |     4 |     2 |     1 |
|  8 |     6 |     6 |     4 |     2 |
|  9 |    10 |    12 |     8 |     4 |
| 10 |    16 |    22 |    14 |     8 |
| 11 |    26 |    40 |    28 |    16 |
| 12 |    42 |    74 |    54 |    30 |
| 13 |    68 |   136 |   104 |    60 |
| 14 |   110 |   250 |   200 |   118 |
| 15 |   178 |   460 |   386 |   232 |
| 16 |   288 |   846 |   744 |   456 | &lt;--- maximum length with binary encoding
| 17 |   466 |  1556 |  1434 |   896 |
| 18 |   754 |  2862 |  2764 |  1762 |
| 19 |  1220 |  5264 |  5328 |  3464 |
| 20 |  1972 |  9682 | 10270 |  6810 |
| 21 |  3162 | 14614 | 19796 | 13388 |
| 22 |  4924 | 15076 | 16940 | 26320 |
| 23 |  7176 |  9836 |  6384 | 10606 |
| 24 |  9370 |  3864 |  1022 |  1324 |
| 25 | 10540 |   842 |    52 |    38 |
| 26 |  9900 |    84 |       |       |
| 27 |  7570 |     2 |       |       |
| 28 |  4600 |       |       |       |
| 29 |  2160 |       |       |       |
| 30 |   754 |       |       |       |
| 31 |   184 |       |       |       |
| 32 |    28 |       |       |       |
| 33 |     2 |       |       |       |

Using `N=2` are clearly not efficient and using `N>2` does not show significant differences.

Taking `N=3` as default has an average of ~22 bits which is ~40% more storage than 16-bits binary encoded.

# Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/xyzzy/untangle/tags).

# License

This project is licensed under the GNU General Public License v3 - see the [LICENSE](LICENSE) file for details
