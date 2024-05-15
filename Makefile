# Makefile for mini-c compiler
#
# Rohan Kartha – Spring 2024

##### Macros #####
AST = front-end/ast
FLAGS = `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15
LIBS = mini_c_compiler.a
FE = front-end
#IRB = irbuilder
OPT = optimizations
OBJS = semantic-analysis.o ast.o lex.yy.o y.tab.o optimizer.o optimizations.o 
#irbuilder.o

.PHONY: all library compiler syntaxanalyzer clean


##### Make all modules #####
all: compiler


##### 
compiler:
	g++ -g main.c $(FLAGS) $(LIBS) -o compiler


##### Make library of object files
library: syntaxanalyzer front-end optimizations modules

modules: optimizations
	ar cr mini_c_compiler.a $(OBJS)


##### Object file dependencies #####
semantic-analysis.o: $(FE)/semantic-analysis.h
ast.o: $(AST)/ast.h
y.tab.o: $(FE)/y.tab.h 
optimizer.o: $(OPT)/optimizer.h 
optimizations.o: $(OPT)/optimizations.h
#irbuilder.o: $(IRB)/irbuilder.h 


##### Make object files #####ir-builder: optimizations g++ -g -I /usr/include/llvm-c-15 -c $(IRB)/irbuilder.c

optimizations: front-end syntaxanalyzer
	g++ -g -I /usr/include/llvm-c-15 -c $(OPT)/optimizations.c 
	g++ -g -I /usr/include/llvm-c-15 -c $(OPT)/optimizer.c

front-end: syntaxanalyzer
	g++ -g -c $(FE)/semantic-analysis.c $(AST)/ast.c $(FE)/lex.yy.c $(FE)/y.tab.c

syntaxanalyzer: front-end/yacc.y front-end/lex.l
	yacc -d -v -t -Wcounterexamples front-end/yacc.y
	lex -d front-end/lex.l


##### Remove testing output and executable files #####
clean:
	rm -f *.o y.tab.c y.tab.h y.output lex.yy.c
	rm -f compiler
	rm -f test_new.ll test_old.ll