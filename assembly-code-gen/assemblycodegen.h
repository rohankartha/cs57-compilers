#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <unordered_map>
#include <string>
#include "../front-end/ast/ast.h"
#include "../front-end/semantic-analysis.h"
#include <iostream>
#include <stack>
#include <set>
#include <unordered_map>
#include <string>
#include <algorithm>
using namespace std;

void computeLiveness(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, int>* instIndex, unordered_map<LLVMValueRef, array<int, 2>>* liveRange);
unordered_map<LLVMValueRef, string> allocateRegisters(LLVMValueRef function);
void generateAssemblyCode(FILE* fp, LLVMModuleRef m, int localMem, unordered_map<LLVMValueRef, string> registerAssignments);