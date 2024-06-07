# 'Mini-c' Compiler

## Rohan Kartha (rohankartha)

### Description

This repository implements a compiler for a subset of the `c` programming language. The compiler compiles a `.c` file that adheres to several constraints, including:

1. Single static assignment of variables
2. One main function with one `int` parameter
3. Imports two external functions (`print` and `read`)

The compiler is built in four parts:

1. `front-end`: (1) implements lexical analyzer and syntax analyzer; (2) implements semantic analyzer; (3) builds abstract syntax tree and checks for appropiate variable declarations in global + local scopes.
2. `ir-builder`: generates LLVM intermediate representation from abstract syntax tree
3. `optimizations`: performs three local optimizations: (1) constant folding, (2) dead code elimination, (3) common subexpression elimination; and one global optimization: constant propagation
4. `assembly-code-gen`: generates x-32 assembly code from optimized LLVM intermediate representation

### Usage

1. Enter `make clean` to remove executables
2. Enter `make` to generate compiler executable.
3. Enter ```bash ./compiler FILE.c``` in the command line to run compiler
4. See `after_ir_builder.ll` and `after_opt.ll` to view unoptimized and optimized versions of llvm representation
5. See `assembly.asm` for x86 assembly instructions
