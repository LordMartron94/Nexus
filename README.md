# nexus

Small C library.

## Build

```bash
cmake -S . -B build -Dnexus_BUILD_APP=ON -Dnexus_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```
