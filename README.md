# 'Mini-c' Compiler

## Rohan Kartha (rohankartha)

### Usage

1. Enter `make clean` to remove executables
2. Enter `make` to generate compiler executable.
3. Enter ```{bash} ./compiler FILE.c``` in the command line to run compiler
4. See `after_ir_builder.ll` and `after_opt.ll` to view unoptimized and optimized versions of llvm representation
5. See `assembly.asm` for x86 assembly instructions
