# Macros
LEXFILENAME = lex
LEXOUTPUT = lex.yy.c
YACCFILENAME = yacc
YACCOUTPUT = y.tab.c

.PHONY: clean test

# Make syntax analyzer from Lex and Yacc
syntaxanalyzer: $(YACCFILENAME).y $(LEXFILENAME).l
	yacc -d $(YACCFILENAME).y
	lex $(LEXFILENAME).l
	gcc $(LEXOUTPUT) $(YACCOUTPUT) -o syntaxanalyzer

test: 
	./syntaxanalyzer test-input.txt

clean:
	rm -rf syntaxanalyzer lex.yy.c y.tab.c y.tab.h
