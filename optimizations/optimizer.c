#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "optimizations.h"

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


void walkBBInstructions(LLVMBasicBlockRef bb){
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction;
  				instruction = LLVMGetNextInstruction(instruction)) {

                    // Local optimizations
                    removeCommonSubexpression(bb);
                    printf("TEST");
                    




		}
}


void walkBasicblocks(LLVMValueRef function){
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
		printf("In basic block\n");

		//walkBBInstructions(basicBlock);

        // Local optimizations
        removeCommonSubexpression(basicBlock);
	
	}
}


void walkFunctions(LLVMModuleRef module){
	for (LLVMValueRef function =  LLVMGetFirstFunction(module); 
			function; 
			function = LLVMGetNextFunction(function)) {

		const char* funcName = LLVMGetValueName(function);	

		printf("Function Name: %s\n", funcName);

		walkBasicblocks(function);
 	}
}


void walkGlobalValues(LLVMModuleRef module){
	for (LLVMValueRef gVal =  LLVMGetFirstGlobal(module);
                        gVal;
                        gVal = LLVMGetNextGlobal(gVal)) {

                const char* gName = LLVMGetValueName(gVal);
                printf("Global variable name: %s\n", gName);
        }
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
		walkGlobalValues(m);
		walkFunctions(m);
		LLVMPrintModuleToFile (m, "test_new.ll", NULL);
	}
	else {
	    fprintf(stderr, "m is NULL\n");
	}
	
	return 0;

	// LLVMValueKind LLVMGetValueKind(LLVMValueRef Val);
	// LLVMTypeRef LLVMTypeOf(LLVMValueRef Val);








}
