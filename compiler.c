/**
 * compiler.c – Driver program for the mini-c compiler
 * 
 * Rohan Kartha, April 2024
 * 
*/



/**************** Dependencies ****************/
#include "front-end/ast/ast.h"
#include "y.tab.h"
#include <stdlib.h>
#include <stdio.h>
#include <cstddef>
#include <vector>
#include "front-end/semantic-analysis.h"
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "optimizations/optimizations.h"
#include "optimizations/optimizer.h"
#include <unordered_map>
#include <set>
#include <string.h>
#include "irbuilder/irbuilder.h"
#include "assembly-code-gen/assemblycodegen.h"
extern FILE* yyin;
extern int yylex_destroy();
using namespace std;
#define prt(x) if(x) { printf("%s\n", x); }



/**************** main ****************/
int main(int argc, char* argv[]) 
{
    /* Section 1: Front-end */
    char* fileName;

    // Open mini-c file
    if (argc == 2) {
        yyin = fopen(argv[1], "r");

        fileName = argv[1];

        if (yyin == NULL) {
            fprintf(stderr, "File open error");
            return 1;
        }
    }

    // Conducting lexical and syntax analysis of mini-c file
    yyparse();
    if (argc == 2) {
        fclose(yyin);
    }
    yylex_destroy();

    // Conducting semantic analysis
    bool result = semanticAnalysis();

    if (result) {
        printf("Semantic analysis passed\n");
    }
    else {
        printf("Semantic analysis NOT passed\n");
    }


    /* Section 2: IR builder */
    LLVMModuleRef m3 = readAstTree(fileName);
    LLVMPrintModuleToFile (m3, "after_ir_builder.ll", NULL);


    /* Section 3: Optimizations */
	walkFunctions(m3);
    LLVMPrintModuleToFile (m3, "after_opt.ll", NULL);


    /* Section 4: Assembly code generation */
    LLVMValueRef function = LLVMGetLastFunction(m3);

    unordered_map<LLVMValueRef, string> registerAssignments = allocateRegisters(function);
    FILE* fp = fopen("assembly.asm", "w");
    generateAssemblyCode(fp, function, registerAssignments);

    // Freeing memory
    freeNode(root);
    LLVMDisposeModule(m3);
	LLVMShutdown();

    return 0;
}