# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

```
2020-07-16 23:04:20 Added `div.c` to research streaming `DIV`.
2020-07-12 23:00:59 Upgraded `srun3` with memory port (sequential access) and `ALU` for core opcodes/instructions.
2020-07-10 23:35:46 Renamed `srun3.c` into `srun3.cc`.
```

## [Release 1.1.0] 2020-07-10 22:48:53

Positive+Negative numbers indicated by polarity end-of-sequence marker.  
`ADD/SUB/LSL/LSR/AND/XOR/OR` operators.  
Full Streaming except for `LSL/LSR` right-hand-side.  

```
2020-07-08 23:01:30 Added `ADD/SUB/LSL/LSR`.
2020-07-07 22:40:54 Added `AND/XOR/OR`.
2020-07-04 22:39:55 Abandoning `urun2.c` in favor of `srun3.c`.
2020-07-01 22:40:54 Added `LSL/LSR/AND/XOR/OR`.
2020-07-01 12:57:42 Third implementation of `OR` in `urun2.c` using loops.
2020-06-30 01:19:40 Second implementation of `OR` in `urun2.c` using generated states.
2020-06-30 01:19:40 Initial implementation of `OR` in `urun2.c`.
2020-06-29 14:07:33 Added `ufrequency.c` for unsigned encoding.
                    Renamed `frequency.c` into `sfrequency.c` for signed encoding. 
```

## Release 1.0.0 2020-06-29 13:23:01

Basic proof of concept.  
Signed variable-width numbers with configurable runlength terminator.  

```
2020-06-29 13:03:05 Added support for negative numbers.
2020-06-26 17:06:38 Fixed incomplete terminator.
2020-06-26 15:59:58 Added `frequency.c`.
2020-06-24 00:29:02 Initial commit.
```

[Unreleased]: /RockingShip/armonika/compare/v1.1.0...HEAD
[1.1.0]: /RockingShip/armonika/compare/v1.0.0...v1.1.0
