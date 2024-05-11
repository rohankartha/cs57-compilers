/**
 * optimizations.h
 * 
 * Rohan Kartha – May 2024
 * 
*/

/***************** dependencies ***********************/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
using namespace std;

/***************** global type declarations ***********************/
typedef struct basicBlockSets {
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> genSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> killSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> inSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> outSets;
} basicBlockSets_t;

/***************** global function declarations ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb);
bool constantFolding(LLVMValueRef function);
bool constantPropagation(LLVMValueRef function);
bool cleanDeadCode(LLVMValueRef function);