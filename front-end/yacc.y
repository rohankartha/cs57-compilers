%{
    #include <stdio.h>
    #include <stdbool.h>
    extern int yylex();
    extern int yylex_destroy();
    extern int yywrap();
    int yyerror(char *);
    extern FILE* yyin;
%}
%union{
    int ival;
    char* string;
    bool boolean;
}

%token IF WHILE ELSE RETURN INTEGER EXTERN VOID
%token <ival> NUM
%token<string> VARIABLE

%token ADD SUBTRACT MULT DIV
/* %nonassoc ADD SUBTRACT MULT DIV */

%token GREATER LESS EQUAL
%token LEFTPAREN RIGHTPAREN LEFTCURL RIGHTCURL
%token SEMICOLON

%type <ival> expression
%type <boolean> condition
%type <string> variable


/* %token PRINT
%token READ */
%%
/* 
statements : statements statement
           | statement */



expression : NUM {$$ = $1;}
           | expression ' ' ADD ' ' expression {$$ = $1 + $5;}
           | expression ' ' SUBTRACT ' ' expression {$$ = $1 - $5;}
           | expression ' ' MULT ' ' expression {$$ = $1 * $5;}
           | expression ' ' DIV ' ' expression {$$ = $1 / $5;}

condition  : 
           | expression ' ' GREATER ' ' expression {$$ = $1 > $5;}
           | expression ' ' LESS ' ' expression {$$ = $1 < $5;}
           | expression ' ' EQUAL ' ' expression {$$ = $1 == $5;}


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

