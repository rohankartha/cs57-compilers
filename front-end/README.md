# CS57 Compiler Front-end

## Rohan Kartha (rohankartha)

### front-end

`front-end` implements the lexical analyzer, syntax analyzer, and semantic analyzer of the mini-c compiler. The lexical analyzer is generated using `lex` and the syntax analyzer with `yacc`. The semantic analyzer is implemented with a `stack` of symbol tables. Each table is implemented with a `vector<char*>` object. The semantic analyzer returns a `bool` variable to the compiler driver program `compiler.c`.

#### Files

`/ast`: third-party dependency which provides abstract syntax tree data structure
`compiler.c`: driver program for the mini-c compiler
`lex.l`: defines how character patterns are tokenize
`yacc.y`: defines grammar rules for tokens
`semantic-analysis.c`: checks that variables are declared and used in correct scope
`testing`: contains test mini-c files and empty folder where output is printed when tests are printed.
`parsertest.sh`: testing script for the front-end

```none
|-- Makefile
|-- README.md
|-- ast
|   |-- ast.c
|   `-- ast.h
|-- compiler.c
|-- lex.l
|-- parsertests.sh
|-- semantic-analysis.c
|-- semantic-analysis.h
|-- testing
|   |-- parser_tests
|   |   |-- main.c
|   |   |-- p1.c
|   |   |-- p2.c
|   |   |-- p3.c
|   |   |-- p4.c
|   |   |-- p5.c
|   |   `-- p_bad.c
|   |-- semantic_analysis_tests
|   |   |-- README
|   |   |-- main.c
|   |   |-- p1_bad.c
|   |   |-- p1_good.c
|   |   |-- p2_bad.c
|   |   |-- p2_good.c
|   |   |-- p3_bad.c
|   |   |-- p3_good.c
|   |   `-- p4_bad.c
|   `-- testing-output
`-- yacc.y
```

#### Testing

To test the front-end:

1. Navigate to the directory `/front-end`.
2. Enter `make` to generate `lex.yy.c` and `y.tab.c` from `lex.l` and `yacc.`
3. Enter `make compiler` to make compiler executable.
4. Enter `make test` to run the test script `parsertests.sh`.
5. Navigate to `testing/testing-output` to view testing output.
6. Enter `make clean` to remove executables and output files from directory.