%{
    #include <stdio.h>
    #include "y.tab.h"
%}
%%
test    {
                        printf("Successful\n");
                    }
.
%%
int yywrap() {
    return(0);
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        yyin = fopen(argv[1], "r");

        if (yyin == NULL) {
            printf("ERROR\n");
            return(1);
        }
    }
    yylex();
    return(0);

}