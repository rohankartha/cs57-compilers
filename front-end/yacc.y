%{
    #include <stdio.h>
    extern int yylex();
    extern int yylex_destroy();
    extern int yywrap();
    int yyerror(char *);
    extern FILE* yyin;
%}
%union{
    int ival;
    char* string;
}

/* %token <ival> INTEGER
%token <string> VARIABLE
%token GREATER
%token LESS
%token EQUALS
%token GREATEREQUAL
%token LESSEQUAL */
%token IF
%token WHILE
%token RETURN
%token ELSE
/* %token PRINT
%token READ */
%%
asdf:

%%
int yyerror(char *s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}

int main(int argc, char* argv[]) {
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

