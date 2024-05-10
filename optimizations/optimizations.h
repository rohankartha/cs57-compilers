#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
using namespace std;

/***************** global type declarations ***********************/
typedef struct basicBlockSets {
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> genSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> killSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> inSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> outSets;
    unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predecessors;
} basicBlockSets_t;


bool removeCommonSubexpression(LLVMBasicBlockRef bb);
bool constantFolding(LLVMValueRef function);
bool constantPropagation(LLVMValueRef function);
bool cleanDeadCode(LLVMValueRef function);

// load parameter correctly
