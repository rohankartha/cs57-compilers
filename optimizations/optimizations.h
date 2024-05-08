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

typedef struct deadCodeMap {
    set<LLVMValueRef> deadCode;
} deadCodeMap_t;

deadCodeMap_t removeCommonSubexpression(LLVMBasicBlockRef bb, deadCodeMap_t deadCode);
deadCodeMap_t constantFolding(LLVMValueRef function, deadCodeMap_t deadCode);
bool constantPropagation(LLVMValueRef function);
void cleanDeadCode(LLVMValueRef function, deadCodeMap_t deadCode);