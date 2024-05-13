# 'Mini-c' Compiler

## Rohan Kartha (rohankartha)

### Testing

1. Enter `make library` to generate mini-c compiler library
2. Enter `make` to generate compiler executable.
3. Enter ```bash ./compiler FILE.c FILE.ll``` in the command line to run compiler
4. See `stdout` to view output from syntax + semantic analysis
5. See `test_new.ll` and `test_old.ll` to view unoptimized and optimized versions of llvm representation
6. Enter `make clean` to remove executables and output files from directory