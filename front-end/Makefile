# Macros
LEXFILENAME = lex
LEXOUTPUT = lex.yy.c
YACCFILENAME = yacc
YACCOUTPUT = y.tab.c
AST = ast/ast.c

.PHONY: clean test

# Make syntax analyzer from Lex and Yacc
syntaxanalyzer: $(YACCFILENAME).y $(LEXFILENAME).l 
	yacc -d -v -t -Wcounterexamples $(YACCFILENAME).y
	lex -d $(LEXFILENAME).l
	g++ $(AST) $(LEXOUTPUT) $(YACCOUTPUT) -o syntaxanalyzer


# syntaxanalyzer: $(YACCFILENAME).y $(LEXFILENAME).l 
# 	yacc -v -d -t --debug -Wcounterexamples $(YACCFILENAME).y
# 	lex -d $(LEXFILENAME).l
# 	gcc $(LEXOUTPUT) $(YACCOUTPUT) -o syntaxanalyzer





test: 
	./syntaxanalyzer parser_tests/p5.c 2> error.txt

clean:
	rm -rf syntaxanalyzer lex.yy.c y.tab.c y.tab.h y.output lex.out error.txt


