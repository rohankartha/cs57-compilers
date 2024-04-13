%{
    #include <stdio.h>
    #include <stdbool.h>
    #include <string.h>
    #include <cstddef>
    #include <vector>
    #include "ast/ast.h"

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
    astNode* nodeptr;
}

%token IF WHILE ELSE RETURN INTEGER EXTERN VOID PRINT READ FUNC
%token <ival> NUM
%token<svar> VARIABLE

%token ADD SUBTRACT MULT DIV
%left ADD SUBTRACT 
%left MULT DIV

%token GREATER LESS EQUAL
%token LPAREN RPAREN LCURL RCURL
%token SEMICOLON


%type <boolean> condition
%type <nodeptr> block statement declaration expression term


/* add to statements: custom functions */
%%

statements : statements statement
           | statement 

statement  : EXTERN VOID PRINT LPAREN INTEGER RPAREN SEMICOLON {}
           | EXTERN INTEGER READ LPAREN RPAREN SEMICOLON {}
           | RETURN LPAREN term RPAREN SEMICOLON {}
           | RETURN term SEMICOLON {}
           | RETURN expression SEMICOLON {}
           | INTEGER VARIABLE EQUAL term {}
           | INTEGER VARIABLE SEMICOLON {}
           | VARIABLE EQUAL term SEMICOLON {}
           | VARIABLE EQUAL expression SEMICOLON {}
           | INTEGER FUNC LPAREN INTEGER VARIABLE RPAREN LCURL statements RCURL {}
           | WHILE LPAREN condition RPAREN LCURL statements RCURL {}
           | IF LPAREN condition RPAREN LCURL statements RCURL {}
           | IF LPAREN condition RPAREN LCURL statements RCURL ELSE LCURL statements RCURL {}
           | function SEMICOLON

function   : READ LPAREN RPAREN {}
           | VARIABLE EQUAL READ LPAREN RPAREN {} 
           | PRINT LPAREN term RPAREN {}   

expression  : term ADD term           { $$ = createBExpr($1, $3, add); }
            | term SUBTRACT term      { $$ = createBExpr($1, $3, sub); }
            | term MULT term          { $$ = createBExpr($1, $3, mul); }
            | term DIV term           { $$ = createBExpr($1, $3, divide); }

condition   : term GREATER term {}
            | term LESS term {}
            | term EQUAL term {}

term        : NUM               { $$ = createCnst($1);}
            | VARIABLE          { $$ = createVar($1);} 



 
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

