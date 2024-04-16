# CS57 Compiler Front-end

## Rohan Kartha (rohankartha)

### front-end

### Testing

To test the front-end:

1. Navigate to the directory `/front-end`.
2. Enter `make` to generate `lex.yy.c` and `y.tab.c` from `lex.l` and `yacc.`
3. Enter `make compiler` to make compiler executable.
4. Enter `make test` to run the test script `parsertests.sh`.
5. Navigate to `testing/testing-output` to view testing output.
6. Enter `make clean` to remove executables and output files from directory.