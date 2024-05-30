/***************** dependencies ***********************/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <unordered_map>
#include <string>
#include "../front-end/ast/ast.h"
using namespace std;

/***************** local-global function declarations ***********************/
void extractStatementList(astBlock block, stack<unordered_map<string, LLVMValueRef>> varNameStack,
        unordered_map<string, int>* nameFreqMap, unordered_map<string, LLVMValueRef> varNameMap);
unordered_map<string, LLVMValueRef> renameVariables(unordered_map<string, LLVMValueRef> varNameMap, 
        unordered_map<string, int>* nameFreqMap, astNode* node, 
        stack<unordered_map<string, LLVMValueRef>> varNameStack);
unordered_map<string, LLVMValueRef> createUniqueVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef> varNameMap, unordered_map<string, int>* nameFreqMap);
LLVMModuleRef readAstTree(char* filename);
unordered_map<string, LLVMValueRef> replaceWithUnique(astNode* node, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap);
char* replaceWithUniqueHelper(string oldVarName, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap);



LLVMModuleRef buildIR(unordered_map<string, int>* nameFreqMap, char* filename);
