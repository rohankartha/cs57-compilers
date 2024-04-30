#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
using namespace std;
bool removeCommonSubexpression(LLVMBasicBlockRef bb);
bool constantFolding(LLVMValueRef function);
set<LLVMValueRef> computeGen(LLVMBasicBlockRef bb);