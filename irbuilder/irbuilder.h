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

unordered_map<string, LLVMValueRef> renameVariables(astNode* root);
string createTemporaryVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef>* varNameMap, unordered_map<string, int>* nameFreqMap);