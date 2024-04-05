%{
    #include <stdio.h>
    #include "y.tab.h"
%}
%%
[0-9]+      {
                printf("INTEGER\n");
            }
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
                else {
                    printf("WORD\n");
                }
            }
[ \t\n]     { 
                return yytext[0]; 
            }
.           {
                printf("Error\n");
            }
%%
int yywrap() {
    return(0);
}

// int main(int argc, char* argv[]) {
//     if (argc == 2) {
//         yyin = fopen(argv[1], "r");

//         if (yyin == NULL) {
//             printf("ERROR\n");
//             return(1);
//         }
//     }
//     yylex();
//     return(0);

// }