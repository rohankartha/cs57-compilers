/**
 * irbuilder.c
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
#include <unordered_map>
#include <string>
#include "../front-end/ast/ast.h"
using namespace std;

/***************** local-global function declarations ***********************/
unordered_map<string, LLVMValueRef> renameVariables(astNode* root);
string createTemporaryVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef>* varNameMap, unordered_map<string, int>* nameFreqMap);


/***************** renameVariables ***********************/
unordered_map<string, LLVMValueRef> renameVariables(astNode* root) 
{

    // Initializing maps to store variables, alloc instructions
    unordered_map<string, LLVMValueRef>* varNameMap;
    unordered_map<string, int>* nameFreqMap;

    // Extracting block statement node from root node
    astFunc func = root->func;
    astStmt stmt = (func.body)->stmt;
    astBlock block = stmt.block;

    vector<astNode*> statements = *block.stmt_list;
    for (auto statIter = statements.begin(); statIter != statements.end(); ++statIter) {
        astStmt statement = (*statIter)->stmt;


        // types of statements that may have variables: call, ret, while, if, decl, assign
        switch (statement.type) {

            // param=NULL in ast.h?
            case ast_call:
                astCall call = statement.call;
                if (call.param != NULL) {

                }






                break;









            case ast_ret:
                astRet ret = statement.ret;
                astNode expr = *ret.expr;
                if (expr.type == ast_var) {

                    // Extracting variable name from node and storing in string
                    astVar var = expr.var;
                    string varName(var.name);

                    // Creating and adding temporary variables to the map if required
                    auto varIter = varNameMap->find(varName);
                    if (varIter != varNameMap->end()) {
                        string newVar = createTemporaryVariable(varName, varNameMap, nameFreqMap);
                        varNameMap->insert({newVar, NULL});
                    }
                }
                break;





            case ast_while:
                break;
            case ast_if:
                break;
            case ast_decl:
                break;
            case ast_asgn:
                break;
        }







    }
    
}


/***************** createTemporaryVariable ***********************/
string createTemporaryVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef>* varNameMap, unordered_map<string, int>* nameFreqMap)
{
    if (varNameMap->find(oldVarName) != varNameMap->end()) {
        string newVarName(oldVarName);
        
        // Constructing temporary variable name
        auto nameIter = nameFreqMap->find(oldVarName);
        if (nameIter != nameFreqMap->end()) {

            int count = nameIter->second;
            count = count + 1;
            nameIter->second = count;

            newVarName = newVarName.append(to_string(count));
            return newVarName;
        }
    }
}




