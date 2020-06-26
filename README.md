# Armonika

Encoding/decoding/handling of variable length numbers in bit addressable memory.

Addressing data in memory requires two components: location (address) and size (bit count).

With fixed width memory (multiple of power-of-2 bytes) the maximum storage size is determined at compile-time
whereas with variable-width memory the unrestricted storage size is determined at run-time.

This project is an experimental environment for storing data in bit-addressable memory.
It is intended to beidge the worlds of the `xtools` and `untangle` projects.

The proposed encoding has these features:
 - Streaming.
 - No maximum size.
 - Simple and fast encoding/decoding.
 - Simple and fast length detection.
 - Simple implementation of bitwise and integer arithmetic operators.
 - Easy hardware implementation.

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
| 00000 | 00         | 000      
| 00001 | 100        | 1000     
| 00010 | 0100       | 01000    
| 00011 | 11000      | 11000    
| 00100 | 0011000    | 001000   
| 00101 | 10100      | 101000   
| 00110 | 011000     | 011000   
| 00111 | 110100     | 1110000  
| 01000 | 0010100    | 00011000 
| 01001 | 10011000   | 1001000  
| 01010 | 010100     | 0101000  
| 01011 | 110011000  | 1101000  
| 01100 | 00110100   | 0011000  
| 01101 | 1011000    | 1011000  
| 01110 | 0110100    | 01110000 
| 01111 | 11011000   | 11101000 
| 10000 | 0010011000 | 000101000
| 10001 | 10010100   | 100011000
| 10010 | 010011000  | 01001000 
| 10011 | 110010100  | 11001000 

# Frequency count

Lowest `N` is simplest implementation, higher `N` is greater storage efficiency.
To aid in making a choice the following table contains frequency counts for encoded lengths with different settings of `N`.
Counted are all the values in range 0-65535.

| length | N = 2 | N = 3 | N = 4 | N = 5 | notes |
|-------:|------:|------:|------:|------:|:------|
|  2 |     1 |     0 |     0 |     0 |
|  3 |     1 |     1 |     0 |     0 |
|  4 |     1 |     1 |     1 |     0 |
|  5 |     2 |     2 |     1 |     1 |
|  6 |     3 |     3 |     2 |     1 |
|  7 |     5 |     6 |     4 |     2 |
|  8 |     8 |    11 |     7 |     4 |
|  9 |    13 |    20 |    14 |     8 |
| 10 |    21 |    37 |    27 |    15 |
| 11 |    34 |    68 |    52 |    30 |
| 12 |    55 |   125 |   100 |    59 |
| 13 |    89 |   230 |   193 |   116 |
| 14 |   144 |   423 |   372 |   228 |
| 15 |   233 |   778 |   717 |   448 |
| 16 |   377 |  1431 |  1382 |   881 | &lt;--- maximum length with binary encoding
| 17 |   610 |  2632 |  2664 |  1732 |
| 18 |   987 |  4841 |  5135 |  3405 |
| 19 |  1596 |  8904 |  9898 |  6694 |
| 20 |  2567 | 13793 | 19079 | 13160 |
| 21 |  4043 | 15106 | 17263 | 25872 |
| 22 |  6050 | 10812 |  7178 | 11215 |
| 23 |  8273 |  4846 |  1351 |  1602 |
| 24 |  9955 |  1281 |    95 |    63 |
| 25 | 10220 |   176 |     1 |     0 |
| 26 |  8735 |     9 |     0 |     0 |
| 27 |  6085 |     0 |     0 |     0 |
| 28 |  3380 |     0 |     0 |     0 |
| 29 |  1457 |     0 |     0 |     0 |
| 30 |   469 |     0 |     0 |     0 |
| 31 |   106 |     0 |     0 |     0 |
| 32 |    15 |     0 |     0 |     0 |
| 33 |     1 |     0 |     0 |     0 |

Using `N=2` are clearly not efficient and using `N>2` does not show significant differences.

Taking `N=3` as default has an average of ~21 bits which is ~30% more storage than 16-bits binary encoded.

# Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/xyzzy/untangle/tags).

# License

This project is licensed under the GNU General Public License v3 - see the [LICENSE](LICENSE) file for details
