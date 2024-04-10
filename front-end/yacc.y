%{
    #include <stdio.h>
    #include <stdbool.h>
    #include <string.h>
    extern int yylex();
    extern int yylex_destroy();
    extern int yywrap();
    int yyerror(char *);
    extern FILE* yyin;
    int yydebug = 1;
%}
%union{
    int ival;
    char* string;
    bool boolean;
}

%token IF WHILE ELSE RETURN INTEGER EXTERN VOID PRINT READ FUNC
%token <ival> NUM
%token<string> VARIABLE

%token ADD SUBTRACT MULT DIV
/* %nonassoc ADD SUBTRACT MULT DIV */

%token GREATER LESS EQUAL
%token LEFTPAREN RIGHTPAREN LEFTCURL RIGHTCURL
%token SEMICOLON

%type <ival> expression
%type <boolean> condition


/* %token PRINT
%token READ 
add to statements: custom functions */
%%

statements : statements statement
           | statement 

statement  : WHILE LEFTPAREN condition RIGHTPAREN LEFTCURL statements RIGHTCURL {}
           | IF ' ' LEFTPAREN condition RIGHTPAREN LEFTCURL statements RIGHTCURL {}
           | IF ' ' LEFTPAREN condition RIGHTPAREN LEFTCURL statements RIGHTCURL ELSE ' ' LEFTCURL statements RIGHTCURL {}
           | EXTERN ' ' VOID ' ' PRINT LEFTPAREN INTEGER RIGHTPAREN SEMICOLON {}
           | EXTERN ' ' INTEGER ' ' READ LEFTPAREN RIGHTPAREN SEMICOLON {}
           | RETURN  ' ' LEFTPAREN expression RIGHTPAREN SEMICOLON {}
           | INTEGER ' ' VARIABLE ' ' EQUAL ' ' expression {}
           | INTEGER ' ' VARIABLE SEMICOLON {}
           | VARIABLE ' ' EQUAL ' ' expression {}
           | INTEGER ' ' FUNC LEFTPAREN INTEGER ' ' VARIABLE RIGHTPAREN LEFTCURL statements RIGHTCURL {}

expression : NUM {$$ = $1;}
           | expression ' ' ADD ' ' expression SEMICOLON {$$ = $1 + $5;}
           | expression ' ' SUBTRACT ' ' expression SEMICOLON {$$ = $1 - $5;}
           | expression ' ' MULT ' ' expression SEMICOLON {$$ = $1 * $5;}
           | expression ' ' DIV ' ' expression SEMICOLON {$$ = $1 / $5;}

condition  : expression ' ' GREATER ' ' expression {$$ = $1 > $5;}
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

