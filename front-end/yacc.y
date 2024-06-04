%{
    #include <stdio.h>
    #include <stdbool.h>
    #include <string.h>
    #include <cstddef>
    #include <vector>
    #include "front-end/ast/ast.h"
    #include "front-end/semantic-analysis.h"

    extern int yylex();
    extern int yylex_destroy();
    extern int yywrap();
    extern int yytext;
    int yyerror(const char *);
    extern FILE* yyin;
    extern astNode* root;
%}

%union{
    int ival;
    char* svar;
    astNode* nodeptr;
    vector<astNode*> *node_vector; 
}

%token IF WHILE ELSE RETURN INTEGER EXTERN VOID PRINT READ FUNC
%token <ival> NUM
%token<svar> VARIABLE

%token ADD SUBTRACT MULT DIV
%left ADD SUBTRACT 
%left MULT DIV

%token GREATER LESS EQUAL EXCLAM
%token LPAREN RPAREN LCURL RCURL
%token SEMICOLON

%type <node_vector> declarations statements
%type <nodeptr> block statement declaration expression term function program condition


/* Grammar rules */
%%

program     : EXTERN VOID PRINT LPAREN INTEGER RPAREN SEMICOLON {}
            | program EXTERN INTEGER READ LPAREN RPAREN SEMICOLON {}
            | program INTEGER FUNC LPAREN INTEGER VARIABLE RPAREN block             {
                                                                                        astNode* print_func = createExtern("print");
                                                                                        astNode* read_func = createExtern("read");
                                                                                        astNode* param = createVar($6);
                                                                                        astNode* new_func = createFunc("func", param, $8);
                                                                                        root = createProg(print_func, read_func, new_func);
                                                                                        $$ = root;
                                                                                        free($6);
                                                                                    }

block       : LCURL declarations statements RCURL       {
                                                            vector<astNode*> *block = new vector<astNode*>();
                                                            block->insert(block->end(), $2->begin(), $2->end());
                                                            block->insert(block->end(), $3->begin(), $3->end());
                                                            $$ = createBlock(block);
                                                            delete($2);
                                                            delete($3);
                                                        }
            | LCURL statements RCURL                    { $$ = createBlock($2); }

declarations: declarations declaration   { $$ = $1; $$->push_back($2); }
            | declaration                { $$ = new vector<astNode*>(); $$->push_back($1); }  
 
declaration : INTEGER VARIABLE SEMICOLON { $$ = createDecl($2); free($2); }

statements  : statements statement       { $$ = $1; $$->push_back($2); }
            | statement                  { $$ = new vector<astNode*>(); $$->push_back($1); }

statement   : RETURN LPAREN term RPAREN SEMICOLON                                     { $$ = createRet($3); }
            | RETURN term SEMICOLON                                                   { $$ = createRet($2); }
            | RETURN expression SEMICOLON                                             { $$ = createRet($2); }
            | RETURN LPAREN expression RPAREN SEMICOLON                               { $$ = createRet($3); }
            | term EQUAL term SEMICOLON                                               { $$ = createAsgn($1, $3); }
            | term EQUAL expression SEMICOLON                                         { $$ = createAsgn($1, $3); }
            | WHILE LPAREN condition RPAREN statement                                 { $$ = createWhile($3, $5); }
            | IF LPAREN condition RPAREN statement ELSE statement                     { $$ = createIf($3, $5, $7); }
            | IF LPAREN condition RPAREN statement                                    { $$ = createIf($3, $5, NULL); }
            | block                                                            
            | function SEMICOLON

function    : READ LPAREN RPAREN                    { $$ = createCall("read", NULL); }
            | term EQUAL function                   { $$ = createAsgn($1, $3); } 
            | PRINT LPAREN term RPAREN              { $$ = createCall("print", $3); }   

expression  : term ADD term           { $$ = createBExpr($1, $3, add); }
            | term SUBTRACT term      { $$ = createBExpr($1, $3, sub); }
            | term MULT term          { $$ = createBExpr($1, $3, mul); }
            | term DIV term           { $$ = createBExpr($1, $3, divide); }

condition   : term GREATER term             { $$ = createRExpr($1, $3, gt); }
            | term LESS term                { $$ = createRExpr($1, $3, lt); }
            | term EQUAL EQUAL term         { $$ = createRExpr($1, $4, eq); }
            | term GREATER EQUAL term       { $$ = createRExpr($1, $4, ge); }
            | term LESS EQUAL term          { $$ = createRExpr($1, $4, le); }
            | term EXCLAM EQUAL term        { $$ = createRExpr($1, $4, neq); }

term        : NUM               { $$ = createCnst($1);}
            | VARIABLE          { $$ = createVar($1); free($1); } 
            | SUBTRACT term     { $$ = createUExpr($2, uminus); }

%%
int yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}