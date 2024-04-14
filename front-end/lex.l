%{
    #include <stdio.h>
    #include <stdbool.h>
    #include <string.h>
    #include "ast/ast.h"
    #include "y.tab.h"
%}
%%
    /* numbers */
[0-9]+      {
                yylval.ival = atoi(yytext);
                return NUM;
            }
    /* keywords */
[a-zA-Z]+   {
                if (strcmp("if", yytext) == 0) {
                    return IF;
                }
                else if (strcmp("else", yytext) == 0) {
                    return ELSE;
                }
                else if (strcmp("while", yytext) == 0) {
                    return WHILE;
                }
                else if (strcmp("return", yytext) == 0) {
                    return RETURN;
                }
                else if (strcmp("int", yytext) == 0) {
                    return INTEGER;
                }
                else if (strcmp("void", yytext) == 0) {
                    return VOID;
                }
                else if (strcmp("extern", yytext) == 0) {
                    return EXTERN;
                }
                else if (strcmp("print", yytext) == 0) {
                    return PRINT;
                }
                else if (strcmp("read", yytext) == 0) {
                    return READ;
                }
                else if (strcmp("func", yytext) == 0) {
                    return FUNC;
                }
                else {
                    yylval.svar = (char*) calloc(strlen(yytext) + 1, sizeof(char));
                    strcpy(yylval.svar, yytext);
                    return VARIABLE;
                }
            }
    /* Mathematical operators */
[+\-*/]     {
                if (strcmp("+", yytext) == 0) {
                    return ADD;
                }
                else if (strcmp("-", yytext) == 0) {
                    return SUBTRACT;
                }
                else if (strcmp("*", yytext) == 0) {
                    return MULT;
                }
                else if (strcmp("/", yytext) == 0) {
                    return DIV;
                }
            }
    /* comparisons */
[<>=!]       {
                if (strcmp("<", yytext) == 0) {
                    return LESS;
                }
                else if (strcmp(">", yytext) == 0) {
                    return GREATER;
                }
                else if (strcmp("=", yytext) == 0) {
                    return EQUAL;
                }
                else if (strcmp("!", yytext) == 0) {
                    return EXCLAM;
                }
            }
    /* whitespace */
[\n\t ]      {}
    /* miscellaneous */
[(){};]     {
                if (strcmp("(", yytext) == 0) {
                    return LPAREN;
                }
                else if (strcmp(")", yytext) == 0) {
                    return RPAREN;
                }
                else if (strcmp("{", yytext) == 0) {
                    return LCURL;
                }
                else if (strcmp("}", yytext) == 0) {
                    return RCURL;
                }
                else if (strcmp(";", yytext) == 0) {
                    return SEMICOLON;
                }
            }
.           {
                printf("ERROR\n");
            }
%%
int yywrap() {
    return(1);
}