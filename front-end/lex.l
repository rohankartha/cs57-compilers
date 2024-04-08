%{
    #include <stdio.h>
    #include <stdbool.h>
    #include "y.tab.h"
%}
%%
    /* numbers */
[0-9]+      {
                yylval.ival = atoi(yytext);
                return NUM;
            }
    /* words */
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
                else {
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
[<>=]       {
                if (strcmp("<", yytext) == 0) {
                    return LESS;
                }
                else if (strcmp(">", yytext) == 0) {
                    return GREATER;
                }
                else if (strcmp("=", yytext) == 0) {
                    return EQUAL;
                }
            }
    /* whitespace */
[ \t\n]     { 
                return yytext[0]; 
            }
    /* miscellaneous */
[(){};]     {
                if (strcmp("(", yytext) == 0) {
                    return LEFTPAREN;
                }
                else if (strcmp(")", yytext) == 0) {
                    return RIGHTPAREN;
                }
                else if (strcmp("{", yytext) == 0) {
                    return LEFTCURL;
                }
                else if (strcmp("}", yytext) == 0) {
                    return RIGHTCURL;
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