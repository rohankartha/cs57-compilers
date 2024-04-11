%{
    #include <stdio.h>
    #include <stdbool.h>
    #include <string.h>
    //#include <cstddef>
    //#include <vector>
    //#include "ast/ast.h"

    extern int yylex();
    extern int yylex_destroy();
    extern int yywrap();
    extern int yytext;
    int yyerror(const char *);
    extern FILE* yyin;
%}
%union{
    int ival;
    char* svar;
    bool boolean;
}

%token IF WHILE ELSE RETURN INTEGER EXTERN VOID PRINT READ FUNC
%token <ival> NUM
%token<svar> VARIABLE

%token ADD SUBTRACT MULT DIV
%left ADD SUBTRACT 
%left MULT DIV

%token GREATER LESS EQUAL
%token LEFTPAREN RIGHTPAREN LEFTCURL RIGHTCURL
%token SEMICOLON

%type <ival> expression
%type <boolean> condition


/* add to statements: custom functions */
%%

statements : statements statement
           | statement 

statement  : EXTERN VOID PRINT LEFTPAREN INTEGER RIGHTPAREN SEMICOLON {}
           | EXTERN INTEGER READ LEFTPAREN RIGHTPAREN SEMICOLON {}
           | RETURN LEFTPAREN expression RIGHTPAREN SEMICOLON {}
           | RETURN expression SEMICOLON {}
           | INTEGER VARIABLE EQUAL expression {}
           | INTEGER VARIABLE SEMICOLON {}
           | VARIABLE EQUAL expression SEMICOLON {}
           | INTEGER FUNC LEFTPAREN INTEGER VARIABLE RIGHTPAREN LEFTCURL statements RIGHTCURL {}
           | WHILE LEFTPAREN condition RIGHTPAREN LEFTCURL statements RIGHTCURL {}
           | IF LEFTPAREN condition RIGHTPAREN LEFTCURL statements RIGHTCURL {}
           | IF LEFTPAREN condition RIGHTPAREN LEFTCURL statements RIGHTCURL ELSE LEFTCURL statements RIGHTCURL {}
           | function SEMICOLON

function   : READ LEFTPAREN RIGHTPAREN {}
           | VARIABLE EQUAL READ LEFTPAREN RIGHTPAREN {} 
           | PRINT LEFTPAREN expression RIGHTPAREN {}   

expression : NUM {}
           | VARIABLE {}
           | expression ADD expression {}
           | expression SUBTRACT expression {}
           | expression MULT expression {}
           | expression DIV expression {}

condition  : expression GREATER expression {}
           | expression LESS expression {}
           | expression EQUAL expression {}
 
%%
int yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}

int main(int argc, char* argv[]) {
    #ifdef YYDEBUG
      yydebug = 1;
  #endif
    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            fprintf(stderr, "File open error");
            return 1;
        }
    }
    yyparse();
    if (argc == 2) {
        fclose(yyin);
    }
    yylex_destroy();
    return 0;
}

