# POSIX-only fork of Info-ZIP UnZip 6.0 with security patches

This repository contains a fork of
[Info-ZIP's Unzip](https://infozip.sourceforge.net/UnZip.html).
It is based on the latest official release (UnZip 6.0) but
with improvements to make the code more robust.

See [patches/](patches/) for a list of security patches I applied,
and [CHANGES.md](CHANGES.md) for a high-level description of other changes.

The goal is to create a modern, secure fork that is easy to compile and and run on modern POSIX systems (primarily Linux), without most of the baggage from supporting archaic and esoteric platforms. Further feature development is explicitly *not* a goal, though cleanups that make the code more secure or easier to audit are.

## Building, testing, and installing.

```
% make all
% make check
% make install
```

To create a package, the `prefix` and `DESTDIR` variables are useful.
Consult the comments in [Makefile](Makefile) for details.
