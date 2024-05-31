/**
 * optimizer.h
 * 
 * Rohan Kartha – May 2024
 * 
*/

/***************** dependencies ***********************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
#include <string.h>
using namespace std;


/***************** function declarations ***********************/
LLVMModuleRef createLLVMModel(char * filename);
void walkFunctions(LLVMModuleRef module);