/**
 * irbuilder.c
 * 
 * Rohan Kartha – May 2024
 * 
*/

// can we have while (1){}
// extern functions for part 1
// add unary for preprocessing


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
#include <set>
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
LLVMBasicBlockRef generateIRStatement(astNode* statementNode, LLVMBuilderRef builder, 
        LLVMBasicBlockRef startBB, unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef,
        LLVMValueRef mainFuncRef, int* blockNum);
LLVMValueRef generateIRExpression(astNode* expressionNode, LLVMBuilderRef builder,
        unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef, LLVMBasicBlockRef bb);
string generateNewBBName(LLVMBasicBlockRef bb, int* blockNum);


/***************** local-global variables ***********************/
LLVMValueRef ret_ref;
LLVMBasicBlockRef retBB;
LLVMValueRef printValRef;
LLVMTypeRef printFunc;
LLVMTypeRef readFunc;




















    void printTestSet(set<string> names);
    void printAllocMap(unordered_map<string, LLVMValueRef> allocMap);




// static void printTest(unordered_map<string, LLVMValueRef>* varNameMap);
// static void printTestTwo(unordered_map<string, int>* nameFreqMap);

/*
* Section 1 – Preprocessing
*
*/

/***************** readAstTree ***********************/
LLVMModuleRef readAstTree(char* filename)
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
    varNameMap.insert({param, nullptr});

    // Starting iteration through ast tree
    extractStatementList(block, varNameStack, nameFreqMap, varNameMap);

    LLVMModuleRef module = buildIR(nameFreqMap, filename);
    char* test= LLVMPrintModuleToString(module);

    // // Testing
    printf("%s\n", test);
    fflush(stdout);
    free(test);

    //free(nameFreqMap);

    delete nameFreqMap;

    return module;

    
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
            break;
        }

        /* Case 5: If statement is an return statement, replace uses of variable with appropriate scope variants */
        case ast_ret: {
            varNameMap = replaceWithUnique(node, nameFreqMap, varNameMap);
            break;
        }

        /* Case 6: If statement if a call statement (print) */
        case ast_call: {
            varNameMap = replaceWithUnique(node, nameFreqMap, varNameMap);
            break;
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
        nameFreqMap->insert({oldVarName, 0});
        varNameMap.insert({oldVarName, NULL});
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
                free(statement.asgn.lhs->var.name);
                statement.asgn.lhs->var.name = constructedNameLHS;
            }

            // If rhs is a variable
            if (asgn.rhs->type == ast_var) {
                char* oldVarName = asgn.rhs->var.name;
                char* constructedNameRHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);

                // Updating variable name in ast
                free(statement.asgn.rhs->var.name);
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

        /* Case 5: If statement is an if/else statement, updating any variables with new names in condition statement */
        case ast_if: {
            astIf ifstmt = statement.ifn;
            astNode* cond = ifstmt.cond;
            if (cond->type == ast_rexpr) {

                astRExpr condition = ifstmt.cond->rexpr;
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

        /* Case 6: If statement is a call statement, updating variable name if parameter */
        case ast_call: {
            astCall callStmt = statement.call;

            if (callStmt.param->type == ast_var) {
                char* oldVarName = callStmt.param->var.name;
                char* constructedNameRHS = replaceWithUniqueHelper(oldVarName, nameFreqMap, varNameMap);
                callStmt.param->var.name = constructedNameRHS;  
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
            char* constructedName = (char*) malloc(varName.length() + 1);
            constructedName = strcpy(constructedName, varName.c_str());
            return constructedName;

        }
    }

}















































/*
* Section 2 – Building LLVM IR
*
*/


/***************** buildIR ***********************/
LLVMModuleRef buildIR(unordered_map<string, int>* nameFreqMap, char* filename) 
{
    // Starting at the root
    astProg prog = root->prog;

    // Generating module with target architecture
    const char* moduleName = filename;
    LLVMModuleRef module = LLVMModuleCreateWithName("");
    const char* Triple = "x86_64-pc-linux-gnu";
    LLVMSetTarget(module, Triple);

    // Generating print function 
    LLVMTypeRef printParam = LLVMInt32Type();
    LLVMTypeRef printParamTypes[] = {printParam};
    LLVMTypeRef voidRet = LLVMVoidType();
    printFunc = LLVMFunctionType(voidRet, printParamTypes, 1, false);

    // Generating read function
    LLVMTypeRef readIntRet = LLVMInt32Type();
    LLVMTypeRef readParamTypes[] = {};
    readFunc = LLVMFunctionType(readIntRet, readParamTypes, 0, false);

    // Adding external functions to module
    const char* printFuncName = "print";
    printValRef = LLVMAddFunction(module, printFuncName, printFunc);
    const char* readFuncName = "read";
    LLVMValueRef readValRef = LLVMAddFunction(module, readFuncName, readFunc);

    // Generating main function
    LLVMTypeRef mainParam = LLVMInt32Type();
    LLVMTypeRef mainParamTypes[] = {mainParam};
    LLVMTypeRef mainIntRet = LLVMInt32Type();
    LLVMTypeRef mainFunc = LLVMFunctionType(mainIntRet, mainParamTypes, 1, false);

    // Adding main function to module
    const char* funcName = prog.func->func.name;
    LLVMValueRef mainFuncRef = LLVMAddFunction(module, funcName, mainFunc);

    // Generating IR builder
    LLVMBuilderRef builder = LLVMCreateBuilder();

    // Generating entry basic block and adding it to function
    const char* bbName = "BB0";
    LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(mainFuncRef, bbName);

    // Creating set with names of all parameters and local variables
    set<string> varNames;

    for (auto iter = nameFreqMap->begin(); iter != nameFreqMap->end(); ++iter) {
        string oldName = iter->first;
        int count = iter->second;

        for (int i = 0; i < count + 1; i++) {
            string newName(oldName);

            if (i > 0) {
                newName = newName.append(to_string(i));
            }
            varNames.insert(newName);
        }
    }

    // Setting position of builder to the end of entryBB
    LLVMPositionBuilderAtEnd(builder, entryBB);

    // Creating map to hold variable names and alloc instruction references
    unordered_map<string, LLVMValueRef> var_map;

    for (auto iter = varNames.begin(); iter != varNames.end(); ++iter) {

        // Generating alloc instruction
        LLVMTypeRef intType = LLVMInt32Type();
        string varName = *iter;
        LLVMValueRef allocInstRef = LLVMBuildAlloca(builder, intType, varName.c_str());

        // Setting mem alignment and adding instruction to map
        LLVMSetAlignment(allocInstRef, 4);
        var_map.insert({varName, allocInstRef});
    }

    // Creating reference to return value allocation instruction
    const char* retName = "ret";
    ret_ref = LLVMBuildAlloca(builder, LLVMInt32Type(), retName);

    // Generating store instruction for the function parameter
    LLVMValueRef paramRef = LLVMGetParam(mainFuncRef, 0);
    string paramName(prog.func->func.param->var.name);
    LLVMValueRef allocInstRef = var_map.at(paramName);
    LLVMBuildStore(builder, paramRef, allocInstRef);

    // Generating return basic block for the function
    retBB = LLVMAppendBasicBlock(mainFuncRef, "BBRet");

    // Repositioning builder to end of the return basic block
    LLVMPositionBuilderAtEnd(builder, retBB);

    // Adding load and return instructions to return basic block
    LLVMValueRef loadRef = LLVMBuildLoad2(builder, LLVMInt32Type(), ret_ref, "");
    LLVMValueRef retRef = LLVMBuildRet(builder, loadRef);

    // Retrieving statement list
    astFunc func = (prog.func)->func;
    astStmt stmt = (func.body)->stmt;
    astBlock block = stmt.block;
    vector<astNode*>* statements = block.stmt_list;
    LLVMBasicBlockRef exitBB;

    // Iterating through all statements in the node statement list 
    int blockNum = 0;
    for (auto stmtIter = statements->begin(); stmtIter != statements->end(); ++stmtIter) {

        // Generating LLVM IR for each statement
        astNode* statementNode = *stmtIter;
        exitBB = generateIRStatement(statementNode, builder, entryBB, var_map, readValRef, mainFuncRef, &blockNum);

        entryBB = exitBB;
    }

    // Adding return terminator instruction at end of function if not already present
    LLVMValueRef terminator = LLVMGetBasicBlockTerminator(exitBB);
    if (terminator == NULL) {

        // Positioning builder at end of exitBB
        LLVMPositionBuilderAtEnd(builder, exitBB);

        // Generating unconditional branch to retBB
        LLVMBuildBr(builder, retBB);
    }

    // Creating set of blocks with predecessors
    set<LLVMBasicBlockRef> blocksWithPredecessors; 

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(mainFuncRef); bb != NULL; 
            bb = LLVMGetNextBasicBlock(bb)) {

        LLVMValueRef terminator = LLVMGetBasicBlockTerminator(bb);
        int numSuccessors = LLVMGetNumSuccessors(terminator);

        for (int i = 0; i < numSuccessors; i++) {
            LLVMBasicBlockRef successorBB = LLVMGetSuccessor(terminator, i);
            blocksWithPredecessors.insert(successorBB);
        }
    }

    // Deleting basic blocks with no predecessors
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(mainFuncRef); bb != NULL; 
            bb = LLVMGetNextBasicBlock(bb)) {

        if (bb != LLVMGetFirstBasicBlock(mainFuncRef)) {
            
            auto iter = blocksWithPredecessors.find(bb);
            if (iter == blocksWithPredecessors.end()) {
                LLVMDeleteBasicBlock(bb);
            }
        }
    }

    // Cleaning up memory
    LLVMDisposeBuilder(builder);

    return module;


    








    /*



Get the terminator instruction of exitBB basic block:
if the terminator is NULL (there is no return statement at the end of the function body)
Set the position of the builder to the end of exitBB.
Generate an unconditional branch to retBB.
Remove all basic blocks that do not have any predecessor basic blocks (think breadth-first search (BFS) !!!)
Time for memory cleanup (delete memory from maps, sets, builder)!*/


}









/***************** generateIRStatement ***********************/
LLVMBasicBlockRef generateIRStatement(astNode* statementNode, LLVMBuilderRef builder, 
        LLVMBasicBlockRef startBB, unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef,
        LLVMValueRef mainFuncRef, int* blockNum) 
{
    // Extracting statement from statement node
    astStmt stmt = statementNode->stmt;

    // Generating LLVM IR of statement 
    switch (stmt.type) {

        /* Case 1: Assignment statement */
        case ast_asgn: {

            // Positioning builder at end of start basic block
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generating LLVMValueRef of RHS of assignment statement
            astNode* rhsNode = stmt.asgn.rhs;
            LLVMValueRef rhs = generateIRExpression(rhsNode, builder, var_map, readValRef, startBB);

            // Retrieving variable name from LHS of assignment statement
            astNode* lhsNode = stmt.asgn.lhs;
            string lhsName(lhsNode->var.name);
            LLVMValueRef newStoreInstRef = LLVMBuildStore(builder, rhs, var_map.at(lhsName));

            return startBB;
        }


        /* Case 2: Call statement */
        case ast_call: {

            astCall callStmt = statementNode->stmt.call;

            if (callStmt.param != nullptr) {

                // Setting position of builder to end of startBB
                LLVMPositionBuilderAtEnd(builder, startBB);

                // Generating LLVM expression for parameter and LLVM statement for call statement
                LLVMValueRef paramRef = generateIRExpression(callStmt.param, builder, var_map, readValRef, startBB);
                LLVMValueRef params[] = {paramRef};
                unsigned int numArgs = 1;

                // Generating call statement
                LLVMBuildCall2(builder, printFunc, printValRef, params, numArgs, "");
            }
            return startBB;
        }


        /* Case 3: While statement*/
        case ast_while: {

            // Setting position of builder to the end of startBB
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generating basic block for condition of while loop
            LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(mainFuncRef, generateNewBBName(startBB, blockNum).c_str());

            // Linking startBB to condBB w/ unconditional branch
            LLVMValueRef newUncondBrRef = LLVMBuildBr(builder, condBB);

            // Moving builder to end of condBB
            LLVMPositionBuilderAtEnd(builder, condBB);

            // Extract while condition node from while statement node
            astWhile whileStatement = statementNode->stmt.whilen;
            astNode* whileCondNode = whileStatement.cond;
            LLVMValueRef newRelExprRef = generateIRExpression(whileCondNode, builder, var_map, readValRef, startBB);

            // Generating true basic block
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(mainFuncRef, generateNewBBName(condBB, blockNum).c_str());

            // Generating false basic block
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(mainFuncRef, generateNewBBName(trueBB, blockNum).c_str());

            // Generating conditional branch instruction to determine successor
            LLVMBuildCondBr(builder, newRelExprRef, trueBB, falseBB);

            // Retrieve while loop body node
            astNode* whileBody = whileStatement.body;

            // Generating LLVM IR for while body
            LLVMBasicBlockRef trueExitBB = generateIRStatement(whileBody, builder, trueBB, var_map, readValRef, mainFuncRef, blockNum);

            // Setting position of builder to end of trueExitBB
            LLVMPositionBuilderAtEnd(builder, trueExitBB);

            // Generating unconditional branch linking trueExitBB to condBB
            LLVMBuildBr(builder, condBB);

            return falseBB;
        }


        /* Case 4: If-else/If statement */
        case ast_if: {

            // Setting position of builder to end of startBB
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Extract if condition node from if statement node
            astIf ifStatement = statementNode->stmt.ifn;
            astNode* ifCondNode = ifStatement.cond;
            LLVMValueRef newRelExprRef = generateIRExpression(ifCondNode, builder, var_map, readValRef, startBB);

            // Generating true basic block
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(mainFuncRef, generateNewBBName(startBB, blockNum).c_str());

            // Generating false basic block
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(mainFuncRef, generateNewBBName(trueBB, blockNum).c_str());

            // Generating conditional branch instruction to determine successor
            LLVMBuildCondBr(builder, newRelExprRef, trueBB, falseBB);

            // Retrieving else node from the if statement node
            astNode* elseNode = ifStatement.else_body; 

            // If there is no else statement
            if (elseNode == NULL) {

                // Retrieving if node
                astNode* ifNode = ifStatement.if_body;
                LLVMBasicBlockRef ifExitBB = generateIRStatement(ifNode, builder, trueBB, var_map, readValRef, mainFuncRef, blockNum);

                // Positioning builder at end of ifExitBB
                LLVMPositionBuilderAtEnd(builder, ifExitBB);

                // Adding unconditional branch to falseBB
                LLVMBuildBr(builder, falseBB);

                return falseBB;
            }

            // If there is an else statement
            else {

                // Generate instructions for statements in if-body and else-body
                astNode* ifNode = ifStatement.if_body;
                LLVMBasicBlockRef ifExitBB = generateIRStatement(ifNode, builder, trueBB, var_map, readValRef, mainFuncRef, blockNum);
                LLVMBasicBlockRef elseExitBB = generateIRStatement(elseNode, builder, falseBB, var_map, readValRef, mainFuncRef, blockNum);

                // Generating endBB for if-else statement
                LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(mainFuncRef, generateNewBBName(falseBB, blockNum).c_str());

                // Linking trueBB and falseBB to common basic block
                LLVMPositionBuilderAtEnd(builder, ifExitBB);
                LLVMBuildBr(builder, endBB);
                LLVMPositionBuilderAtEnd(builder, elseExitBB);
                LLVMBuildBr(builder, endBB);

                return endBB;
            }
        }


        /* Case 5: Return statement */
        case ast_ret: {

            // Setting the position of the builder to the end of startBB.
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generating reference to return value
            astRet returnStatement = statementNode->stmt.ret;
            astNode* retNode = returnStatement.expr;
            LLVMValueRef retValRef = generateIRExpression(retNode ,builder, var_map, readValRef, startBB);

            // Storing return value in memory location assigned for return value earlier
            LLVMBuildStore(builder, retValRef, ret_ref);

            // Generating an unconditional branch to endBB
            LLVMBuildBr(builder, retBB);

            // Generating and returning new basic block
            LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(mainFuncRef, generateNewBBName(startBB, blockNum).c_str());

            return endBB;
        }


        /* Case 6: Block statement */
        case ast_block: {

            // Setting prevBB equal to startBB
            LLVMBasicBlockRef prevBB = startBB;
            astBlock blockNode = statementNode->stmt.block;

            // Retrieving list of statements from block
            vector<astNode*> *statementList = blockNode.stmt_list;

            // Iterating through each statement in the block
            for (auto statIter = statementList->begin(); statIter != statementList->end(); ++statIter) {

                astNode* statementNode = *statIter;
                astStmt statement = statementNode->stmt;

                // Generating IR of each statement in the block
                prevBB = generateIRStatement(statementNode, builder, prevBB, var_map, readValRef, mainFuncRef, blockNum);
            }
                
            return prevBB; 
        }
    }

    // If statement type does not satisfy one of the above cases, returning startBB
    return startBB;
}


















/***************** generateIRExpression ***********************/
LLVMValueRef generateIRExpression(astNode* expressionNode, LLVMBuilderRef builder,
        unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef, LLVMBasicBlockRef bb)
{
    switch (expressionNode->type) {

        /* Case 1: Constant node */
        case ast_cnst: {

            // creating a new LLVM reference to constant value
            unsigned long long intValue = expressionNode->cnst.value;
            LLVMValueRef newConstInst = LLVMConstInt(LLVMInt32Type(), intValue, true);
            return newConstInst;
            break;
        }


        /* Case 2: Var node */
        case ast_var: {

            // Retrieving variable name from node
            char* varNameAsChar = expressionNode->var.name;
            string varName(varNameAsChar);

            // Retrieving variable value from memory and creating load instruction
            LLVMValueRef memLoc = var_map.at(varName);
            LLVMValueRef newLoadInstRef = LLVMBuildLoad2(builder, LLVMInt32Type(), memLoc, "");
            
            return newLoadInstRef;
            break;
        }


        /* Case 3: Unary node */
        case ast_uexpr: {

            // Generating LLVM reference for inner expression node
            astNode* innerExpNode = expressionNode->uexpr.expr;
            LLVMValueRef innerExpNodeRef = generateIRExpression(innerExpNode, builder, var_map, readValRef, bb);

            // Generating subtraction instruction with constant 0
            unsigned long long zeroValue = 0;
            LLVMValueRef zeroValueRef = LLVMConstInt(LLVMInt32Type(), zeroValue, true);
            LLVMValueRef subInst = LLVMBuildSub(builder, zeroValueRef, innerExpNodeRef, "");

            return subInst;
            break;
        }


        /* Case 4: Binary node */
        case ast_bexpr: {

            // Generating LLVM reference for lhs inner expression node
            astNode* lhsInnerExpNode = expressionNode->bexpr.lhs;
            LLVMValueRef lhsInnerExpNodeRef = generateIRExpression(lhsInnerExpNode, builder, var_map, readValRef, bb);

            // Generating LLVM reference for rhs inner expression node
            astNode* rhsInnerExpNode = expressionNode->bexpr.rhs;
            LLVMValueRef rhsInnerExpNodeRef = generateIRExpression(rhsInnerExpNode, builder, var_map, readValRef, bb);

            // Extracting type of arithmetic operation
            op_type operation = expressionNode->bexpr.op;

            // Generating LLVM IR arithmetic instruction
            switch (operation) {
                case add: {
                    LLVMValueRef newAddInstRef = LLVMBuildAdd(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newAddInstRef;
                    break;
                }
                
                case sub: {
                    LLVMValueRef newSubInstRef = LLVMBuildSub(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newSubInstRef;
                    break;
                }

                case mul: {
                    LLVMValueRef newMulRef = LLVMBuildMul(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newMulRef;
                    break;
                }

                case divide: {
                    LLVMValueRef newDivRef = LLVMBuildSDiv(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newDivRef;
                    break;
                }
            }
        }


        /* Case 5: Relational node */
        case ast_rexpr: {

            // Generating LLVM reference for lhs inner expression node
            astNode* lhsInnerExpNode = expressionNode->rexpr.lhs;
            LLVMValueRef lhsInnerExpNodeRef = generateIRExpression(lhsInnerExpNode, builder, var_map, readValRef, bb);

            // Generating LLVM reference for rhs inner expression node
            astNode* rhsInnerExpNode = expressionNode->rexpr.rhs;
            LLVMValueRef rhsInnerExpNodeRef = generateIRExpression(rhsInnerExpNode, builder, var_map, readValRef, bb);

            // Generating compare instruction
            rop_type operation = expressionNode->rexpr.op;
            LLVMIntPredicate operationType;
            LLVMValueRef newCmpInstRef;

            switch (operation) {
                case lt: {
                    operationType = LLVMIntSLT;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                    break;
                }

                case gt: {
                    operationType = LLVMIntSLT;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                    break;
                }

                case le: {
                    operationType = LLVMIntSLE;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                    break;
                }

                case ge: {
                    operationType = LLVMIntSGT;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                    break;
                }

                case eq: {
                    operationType = LLVMIntEQ;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                    break;
                }

                case neq: {
                    operationType = LLVMIntNE;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                    break;
                }
            }
            return newCmpInstRef;
        }


        /* Case 6: Call instruction */
        case ast_stmt: {

            if (expressionNode->stmt.type == ast_call) {
                LLVMValueRef params[] = {};
                LLVMValueRef newCallRef = LLVMBuildCall2(builder, readFunc, readValRef, params, 0, "");
                return newCallRef;
            }
            return NULL;
            break;
        }


        /* Case 7: Default */
        default: {
            return NULL;
        }
    }
}


/***************** generateNewBBName ***********************/
string generateNewBBName(LLVMBasicBlockRef bb, int* blockNum) 
{
    // Retrieving name of parent basic block
    const char* parentBBName = LLVMGetBasicBlockName(bb);
    string parentBBNameAsString(parentBBName);

    // Generating new basic block name
    (*blockNum)++;
    string newParentBBName = "BB" + to_string(*blockNum);
    return (newParentBBName);
}





















 /*
                
                LLVMTypeRef testParam = LLVMInt32Type();
    LLVMTypeRef testParamTypes[] = {};
    LLVMTypeRef testIntRet = LLVMVoidType();
    LLVMTypeRef testFunc = LLVMFunctionType(testIntRet, testParamTypes, 1, false);

    LLVMValueRef testVal = LLVMAddFunction(module, "test", testFunc);
    LLVMValueRef testInputs[] = {};

    LLVMValueRef testInst = LLVMBuildCall2(builder, testFunc, testVal, testInputs, 0, "test");*/


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

    // void printTestSet(set<string> names) {
    //     printf("Printing var names\n");
    //     for (auto iter = names.begin(); iter != names.end(); ++iter) {
    //         string name = *iter;

    //         cout << name << endl;
            
    //     }
    // }


    // void printAllocMap(unordered_map<string, LLVMValueRef> allocMap) {
    //     for (auto iter = allocMap.begin(); iter != allocMap.end(); ++iter) {
    //         string name = iter->first;
    //         string allocInst = LLVMPrintValueToString(iter->second);

    //         printf("test\n");
    //         fflush(stdout);

    //         cout << name + " " + allocInst << endl;
    //         fflush(stdout);
    //     }
    // }