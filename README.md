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

# Efficiency:

After the leading "1", every sequence of "000" is replace with "0001" adding one bit, and 3 extra bits (value zero) for terminator.


# Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/xyzzy/untangle/tags).

# License

This project is licensed under the GNU General Public License v3 - see the [LICENSE](LICENSE) file for details
