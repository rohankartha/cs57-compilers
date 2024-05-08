#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "optimizations.h"
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
using namespace std;

#define prt(x) if(x) { printf("%s\n", x); }

/* This function reads the given llvm file and loads the LLVM IR into
	 data-structures that we can works on for optimization phase.
*/
 
LLVMModuleRef createLLVMModel(char * filename)
{
	char *err = 0;
	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);
	if (err != NULL) { 
		prt(err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);
	if (err != NULL) {
		prt(err);
	}
	return m;
}



void walkBasicblocks(LLVMValueRef function){
	deadCodeMap_t codeMap;
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        // Local optimization
        codeMap = removeCommonSubexpression(basicBlock, codeMap);
	}
	// Local optimization
	codeMap = constantFolding(function, codeMap);
	cleanDeadCode(function, codeMap);

	

	// Global optimization
	constantPropagation(function);
}


void walkFunctions(LLVMModuleRef module){
	LLVMValueRef function =  LLVMGetFirstFunction(module); 
	const char* funcName = LLVMGetValueName(function);	

	printf("Function Name: %s\n", funcName);
	walkBasicblocks(function);
	printf("exitmain");
	fflush(stdout);
}


int main(int argc, char** argv)
{
	LLVMModuleRef m;

	if (argc == 2){
		m = createLLVMModel(argv[1]);
	}
	else{
		m = NULL;
		return 1;
	}

	if (m != NULL){
		//LLVMDumpModule(m);
		LLVMPrintModuleToFile (m, "test_old.ll", NULL);
		walkFunctions(m);
		LLVMPrintModuleToFile (m, "test_new.ll", NULL);
	}
	else {
	    fprintf(stderr, "m is NULL\n");
	}
	return 0;
}
