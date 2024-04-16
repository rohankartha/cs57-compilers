#include "ast/ast.h"
#include "y.tab.h"
#include <stdlib.h>
#include <stdio.h>
#include <cstddef>
#include <vector>
#include "semantic-analysis.h"
extern FILE* yyin;
extern int yylex_destroy();

ast_Node* root = NULL;


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

    printNode(root);

    bool result = semanticAnalysis();

    if (result) {
        printf("Successfully parsed\n");
    }
    else {
        printf("Error with parsing\n");
    }

    freeNode(root);


    



    return 0;
}