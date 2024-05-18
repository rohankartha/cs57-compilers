/**
 * irbuilder.c
 * 
 * Rohan Kartha – May 2024
 * 
*/

// can we have while (1){}


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
#include "../front-end/semantic-analysis.h"
#include <iostream>
#include <stack>
using namespace std;


/***************** local-global function declarations ***********************/
void extractStatementList(astBlock block, stack<unordered_map<string, LLVMValueRef>> varNameStack,
        unordered_map<string, int>* nameFreqMap, unordered_map<string, LLVMValueRef> varNameMap);
unordered_map<string, LLVMValueRef> renameVariables(unordered_map<string, LLVMValueRef> varNameMap, 
        unordered_map<string, int>* nameFreqMap, astNode* node, 
        stack<unordered_map<string, LLVMValueRef>> varNameStack);
unordered_map<string, LLVMValueRef> createUniqueVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef> varNameMap, unordered_map<string, int>* nameFreqMap);
void readAstTree();
unordered_map<string, LLVMValueRef> replaceWithUnique(astNode* node, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap);
char* replaceWithUniqueHelper(string oldVarName, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap);





// static void printTest(unordered_map<string, LLVMValueRef>* varNameMap);
// static void printTestTwo(unordered_map<string, int>* nameFreqMap);

/***************** readAstTree ***********************/
void readAstTree()
{
    // Extracting block statement node from root node
    astProg prog = root->prog;
    astFunc func = (prog.func)->func;
    astStmt stmt = (func.body)->stmt;
    astBlock block = stmt.block;

    // Initializing stacks to hold maps for different scopes
    stack<unordered_map<string, LLVMValueRef>> varNameStack;

    // Initializing maps for renaming variables
    unordered_map<string, int>* nameFreqMap = new unordered_map<string, int>;
    unordered_map<string, LLVMValueRef> varNameMap;

    // Extracting parameter variable and adding it to maps
    string param(func.param->var.name);
    nameFreqMap->insert({param, 0});
    varNameMap.insert({param, NULL});

    // Starting iteration through ast tree
    extractStatementList(block, varNameStack, nameFreqMap, varNameMap);
}



/***************** extractStatementList ***********************/
void extractStatementList(astBlock block, stack<unordered_map<string, LLVMValueRef>> varNameStack,
        unordered_map<string, int>* nameFreqMap, unordered_map<string, LLVMValueRef> varNameMap)
{
    // Retrieving list of statements from block
    vector<astNode*> *statementList = block.stmt_list;

    // Iterating through statements to identify and rename shadowed variables
    for (auto statIter = statementList->begin(); statIter != statementList->end(); ++statIter) {

        astNode* statementNode = *statIter;
        astStmt statement = statementNode->stmt;
        varNameMap = renameVariables(varNameMap, nameFreqMap, statementNode, varNameStack);
    }
    return;
}



/***************** renameVariables ***********************/
unordered_map<string, LLVMValueRef> renameVariables(unordered_map<string, LLVMValueRef> varNameMap, 
        unordered_map<string, int>* nameFreqMap, astNode* node, 
        stack<unordered_map<string, LLVMValueRef>> varNameStack)
{
    // Extracting statement from node
    astStmt statement = node->stmt;

    // Iterating through each type of statement
    switch (statement.type) {

        // ADD CALL

        /* Case 1: If statement is a declaration statement, add a new unique variable to the map */
        case ast_decl: {
            astDecl decl = statement.decl;
            string oldVarName(decl.name);
            varNameMap = createUniqueVariable(oldVarName, varNameMap, nameFreqMap);

            // Replacing uses of variable with new name
            varNameMap = replaceWithUnique(node, nameFreqMap, varNameMap);
            break;
        }

        /* Case 2: If statement is an while statement, first replacing any renamed variables in conditions
        with updated names */
        case ast_while: {
            astWhile whilestmt = statement.whilen;
            astNode* cond = whilestmt.cond;
            varNameMap = replaceWithUnique(node, nameFreqMap, varNameMap);

            // If body contains a block statement, extract its list of statements + enter new scope
            astNode* body = whilestmt.body;
            if (body->stmt.type == ast_block) {

                // Saving current states of maps to stack
                varNameStack.push(varNameMap);

                // Enter new scope
                astBlock block = body->stmt.block;
                extractStatementList(block, varNameStack, nameFreqMap, varNameMap);

                // Popping maps from the stack
                if (!varNameStack.empty()) {
                    varNameMap = varNameStack.top();
                    varNameStack.pop();
                }
            }

            // Otherwise, recursively call this function for the single statement
            else {
                astStmt statement = body->stmt;
                varNameMap = renameVariables(varNameMap, nameFreqMap, body, varNameStack);
            }
            break;
        }

        /* Case 3: If statement is an if-else statement, first replacing any renamed variables in conditions
        with updated names */
        case ast_if: {
            varNameMap = replaceWithUnique(node, nameFreqMap, varNameMap);
            astIf ifstmt = statement.ifn;
            astNode* cond = ifstmt.cond;

            // Reading statements in if body
            astNode* ifbody = ifstmt.if_body;

            // If body contains a block statement, extract its list of statements + enter new scope
            if (ifbody->stmt.type == ast_block) {

                // Saving current states of maps to stack
                varNameStack.push(varNameMap);
                
                // Enter new scope
                astBlock block = ifbody->stmt.block;
                extractStatementList(block, varNameStack, nameFreqMap, varNameMap);

                // Popping maps from the stack
                varNameMap = varNameStack.top();

                if (!varNameStack.empty()) {
                    varNameStack.pop();
                }
            }

            // Otherwise, recursively calling this function for the single statement
            else {
                astStmt statement = ifbody->stmt;
                varNameMap = renameVariables(varNameMap, nameFreqMap, ifbody, varNameStack);
            }

            // Reading statements in else body
            if (ifstmt.else_body != NULL) {
                astNode* elsebody = ifstmt.else_body;

                // If else statement contains block, saving current states of maps to stack
                if (elsebody->stmt.type == ast_block) {
                    varNameStack.push(varNameMap);

                    // Enter new scope
                    astBlock block = elsebody->stmt.block;
                    extractStatementList(block, varNameStack, nameFreqMap, varNameMap);

                    // Popping maps from the stack
                    varNameMap = varNameStack.top();

                    if (!varNameStack.empty()) {
                        varNameStack.pop();
                    }
                }

                // Otherwise, recursively call this function for the single statement
                else {
                    astStmt statement = elsebody->stmt;
                    varNameMap = renameVariables(varNameMap, nameFreqMap, elsebody, varNameStack);
                }
                break;
            }
        }

        /* Case 4: If statement is an assignment statement, replace uses of variable with appropriate scope variants */
        case ast_asgn: {
            varNameMap = replaceWithUnique(node, nameFreqMap, varNameMap);
        }

        /* Case 5: If statement is an return statement, replace uses of variable with appropriate scope variants */
        case ast_ret: {
            varNameMap = replaceWithUnique(node, nameFreqMap, varNameMap);
        }
    }
    return varNameMap;
}



/***************** createUniqueVariable ***********************/
unordered_map<string, LLVMValueRef> createUniqueVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef> varNameMap, unordered_map<string, int>* nameFreqMap)
{
    // Retrieving first letter of original variable name
    char firstLetter = oldVarName.at(0);
    unordered_map<string, LLVMValueRef> varNameMapCopy(varNameMap);

    // Removing variables in the map with the same first letter
    for (auto iter = varNameMap.begin(); iter != varNameMap.end(); ++iter) {
        string varName = iter->first;

        if (firstLetter == varName[0]) {
            varNameMapCopy.erase(varName);
        }
    }

    // Updating map to show the deletions
    varNameMap = varNameMapCopy;

    // Retrieving frequency from character frequency map
    auto iter = nameFreqMap->find(oldVarName);
    if (iter != nameFreqMap->end()) {
        string newVarName(oldVarName);
            
        // Constructing temporary variable name and adding it to the map
        auto nameIter = nameFreqMap->find(oldVarName);
        if (nameIter != nameFreqMap->end()) {

            int count = nameIter->second;
            count = count + 1;
            nameIter->second = count;

            newVarName = newVarName.append(to_string(count));
            varNameMap.insert({newVarName, NULL});
        }
    }

    // If variable name is not in the frequency map, adding it
    else {
        varNameMap.insert({oldVarName, NULL});
        nameFreqMap->insert({oldVarName, 0});
    }
    return varNameMap;
}



/***************** replaceWithUnique ***********************/
unordered_map<string, LLVMValueRef> replaceWithUnique(astNode* node, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap)
{
    // Extracting statement from node
    astStmt statement = node->stmt;

    // Defining instructions per statement type
    switch(statement.type) {

        /* Case 1: If statement is a declaration statement, replace variable with new name */
        case ast_decl: {
            astDecl decl = statement.decl;
            char* oldVarName = statement.decl.name;
            char* constructedName = replaceWithUniqueHelper(decl.name, nameFreqMap, varNameMap);

            // Updating variable name in ast
            node->stmt.decl.name = constructedName;
            break;
        }

        /* Case 2: If statement is an assignment statement, update lhs and rhs with new names */
        case ast_asgn: {
            astAsgn asgn = statement.asgn;
            if (asgn.lhs->type == ast_var) {
                char* oldVarName = asgn.lhs->var.name;
                char* constructedNameLHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);

                // Updating variable name in ast
                statement.asgn.lhs->var.name = constructedNameLHS;
            }

            // If rhs is a variable
            if (asgn.rhs->type == ast_var) {
                char* oldVarName = asgn.rhs->var.name;
                char* constructedNameRHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);

                // Updating variable name in ast
                statement.asgn.rhs->var.name = constructedNameRHS;
            }

            // If rhs is a binary expression
            else {
                if (asgn.rhs->type == ast_bexpr) {
                    astBExpr binaryexpr = asgn.rhs->bexpr;

                    if (binaryexpr.lhs->type == ast_var) {
                        char* oldVarName = binaryexpr.lhs->var.name;
                        char* constructedNameLHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);

                        // Updating variable name in ast
                        binaryexpr.lhs->var.name = constructedNameLHS;  
                    }


                    if (binaryexpr.rhs->type == ast_var) {
                        char* oldVarName = binaryexpr.rhs->var.name;
                        char* constructedNameRHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);

                        // Updating variable name in ast
                        binaryexpr.rhs->var.name = constructedNameRHS;  
                    }
                }
            }
            break;
        }

        /* Case 3: If statement is a return statement, updating any variables with new names */
        case ast_ret: {
            astRet ret = statement.ret;
            astNode expr = *ret.expr;

            if (expr.type == ast_bexpr) {
                astBExpr binaryexpr = expr.bexpr;

                if (binaryexpr.lhs->type == ast_var) {
                    char* oldVarName = binaryexpr.lhs->var.name;
                    char* constructedNameLHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);
                    binaryexpr.lhs->var.name = constructedNameLHS;  
                }


                if (binaryexpr.rhs->type == ast_var) {
                    char* oldVarName = binaryexpr.rhs->var.name;
                    char* constructedNameRHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);
                    binaryexpr.rhs->var.name = constructedNameRHS;  
                }
            }
            break;
        }

        /* Case 4: If statement is a while statement, updating any variables with new names in condition statement */
        case ast_while: {
            astWhile whilestmt = statement.whilen;
            astNode* cond = whilestmt.cond;
            if (cond->type == ast_rexpr) {

                astRExpr condition = whilestmt.cond->rexpr;
                if ((condition.lhs)->type == ast_var) {

                    char* oldVarName = condition.lhs->var.name;
                    char* constructedNameLHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);
                    condition.lhs->var.name = constructedNameLHS;
                }

                if ((condition.rhs)->type == ast_var) {
                    char* oldVarName = condition.rhs->var.name;
                    char* constructedNameRHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);
                    condition.rhs->var.name = constructedNameRHS;
                }
            }
            break;
        }
    }
    return varNameMap;
}



/***************** replaceWithUniqueHelper ***********************/
char* replaceWithUniqueHelper(string oldVarName, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap)
{
    // Retrieving first letter of original variable name
    char firstLetter = oldVarName.at(0);

    // Retrieving new name from variable name map and returning 
    for (auto iter = varNameMap.begin(); iter != varNameMap.end(); ++iter) {
        string varName = iter->first;

        if (firstLetter == varName[0]) {
            char* constructedName = (char*) malloc(varName.length());
            constructedName = strcpy(constructedName, varName.c_str());
            return constructedName;

        }
    }
}































// static void printTest(unordered_map<string, LLVMValueRef>* varNameMap) {
//     printf("------------\n");

//     for (auto iter = varNameMap->begin(); iter != varNameMap->end(); ++iter) {
//         string test = iter->first;


//         cout << test << endl;
//     }
//     printf("------------\n");

// }


// static void printTestTwo(unordered_map<string, int>* nameFreqMap) {
//     printf("------------\n");

//     for (auto iter = nameFreqMap->begin(); iter != nameFreqMap->end(); ++iter) {
//         string test = iter->first;
//         int freq = iter->second;


//         cout << test;
//         cout << " ";
//         cout << freq << endl;
//     }
//     printf("------------\n");

// }
