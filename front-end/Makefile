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

# Testing parser and semantic analyzer
test: 
	./parsertests.sh

# Checking for memory leaks
valgrind:
	$(VALGRIND) ./compiler testing/semantic_analysis_tests/p3_good.c

# Remove output and object files
clean:
	rm -rf syntaxanalyzer lex.yy.c y.tab.c y.tab.h y.output lex.out testing/testing-output/* compiler vgcore.2591482


