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


void walkBBInstructions(LLVMBasicBlockRef bb){
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction;
  				instruction = LLVMGetNextInstruction(instruction)) {

                    // Local optimizations
                    //removeCommonSubexpression(bb);
                    // printf("TEST");

					// set<LLVMValueRef> test = computeGen(bb);

					// for (auto it = test.begin(); it != test.end(); ++it) {
					// 	LLVMValueRef testRef = *it;
					// 	char* testString = LLVMPrintValueToString(testRef);
					// 	printf("%s\n", testString);
					// }
                    




		}
}


void walkBasicblocks(LLVMValueRef function){
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
		//printf("In basic block\n");

		walkBBInstructions(basicBlock);

        // Local optimizations
		//printf("Remove common subexpression: \n");
        removeCommonSubexpression(basicBlock);
	
	}
}


void walkFunctions(LLVMModuleRef module){
	LLVMValueRef function =  LLVMGetFirstFunction(module); 
			

	const char* funcName = LLVMGetValueName(function);	

	printf("Function Name: %s\n", funcName);

	walkBasicblocks(function);

		// Testing
		//constantFolding(function);
	bool cpResult = constantPropagation(function);
	printf("exitmain");
	fflush(stdout);
 	
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
}
