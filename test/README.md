# Constant testing

Constant testing facility was originally for testing actions during compile time,
which makes sure that there is no UB in them. However, the type erasure implementation,
at the time, requires C++26 to be able to run during compile time.
We want to move to C++23 which makes constant testing impossible.
In the long run when C++26 is widely supported we want to move back to it and again
enable constant testing, so constant testing is replaced by non-constant testing
which is written such that minimizes indention changes.

This still applies even if we start using shared pointers, as they too require
C++26 for compile time support.
