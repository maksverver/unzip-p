# POSIX-only fork of Info-ZIP UnZip 6.0 with security patches

This repository contains an up-to-date version of Info-Tools unzip with security patches applied and with unnecessary cruft removed.

The goal is to create a single repository with just the code necessary to compile and run on POSIX systems (primarily Linux), without most of the baggage from supporting archaic and esoteric platforms, and with the confidence that the code is reasonably simple and well-vetted. Further feature development is explicitly *not* a goal.

See [patches/](patches/) for details about which external patches have been applied.

a5b558cfef0dddab1fad1a1a35cef71c3945bb6e
ZIP bomb vulnerability (CVE-2024-0450) was addressed with [my own patch](https://github.com/maksverver/unzip/commit/0c9dab6dc00b873791087ead0d7e54494bfc71f1), which is simple and effective. By comparison, Mark Adler's solution is vulnerable to [quadratic runtime on maliciously constructed files](https://github.com/madler/unzip/issues/13).

EOF.
