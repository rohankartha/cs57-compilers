# Macros
LEXOUTPUT = lex.yy.c
YACCOUTPUT = y.tab.c
COMPILER = compiler.c
AST = ast/ast.c
YACCHEADER = y.tab.h
SEMANALYSIS = semantic-analysis.c
VALGRIND = valgrind -s --leak-check=full --show-leak-kinds=all

.PHONY: clean test

# Generate yyparse from Lex and Yacc
syntaxanalyzer: yacc.y lex.l
	yacc -d -v -t -Wcounterexamples yacc.y
	lex -d lex.l

# Build and execute the compiler program
compiler: $(YACCHEADER)
	g++ -g $(COMPILER) $(SEMANALYSIS) $(AST) $(YACCOUTPUT) $(LEXOUTPUT) -o compiler

test: 
	./parsertests.sh

valgrind:
	$(VALGRIND) ./compiler parser_tests/p1.c

clean:
	rm -rf syntaxanalyzer lex.yy.c y.tab.c y.tab.h y.output lex.out error1.txt error2.txt error3.txt error4.txt error5.txt testtree1.txt testtree2.txt testtree3.txt testtree4.txt testtree5.txt compiler


