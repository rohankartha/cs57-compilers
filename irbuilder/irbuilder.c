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
void readAstTree(char* filename);
unordered_map<string, LLVMValueRef> replaceWithUnique(astNode* node, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap);
char* replaceWithUniqueHelper(string oldVarName, unordered_map<string, int>* nameFreqMap, unordered_map<string, 
        LLVMValueRef> varNameMap);










LLVMModuleRef buildIR(unordered_map<string, int>* nameFreqMap, char* filename);
LLVMBasicBlockRef generateIRStatement(astNode* statementNode, LLVMBuilderRef builder, 
        LLVMBasicBlockRef startBB, unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef,
        LLVMValueRef mainFuncRef);
LLVMValueRef generateIRExpression(astNode* expressionNode, LLVMBuilderRef builder,
        unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef, LLVMBasicBlockRef bb);

const char* generateNewBBName(LLVMBasicBlockRef bb);








/***************** local-global variables ***********************/
LLVMValueRef ret_ref;
LLVMBasicBlockRef retBB;









    void printTestSet(set<string> names);
    void printAllocMap(unordered_map<string, LLVMValueRef> allocMap);




// static void printTest(unordered_map<string, LLVMValueRef>* varNameMap);
// static void printTestTwo(unordered_map<string, int>* nameFreqMap);

/*
* Section 1 – Preprocessing
*
*/

/***************** readAstTree ***********************/
void readAstTree(char* filename)
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

    LLVMModuleRef module = buildIR(nameFreqMap, filename);
    
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















































/*
* Section 2 – Building LLVM IR
*
*/

// function attributes??




/***************** buildIR ***********************/
LLVMModuleRef buildIR(unordered_map<string, int>* nameFreqMap, char* filename) 
{
    // Starting at the root
    astProg prog = root->prog;

    // Generating module with target architecture
    const char* moduleName = filename;
    LLVMModuleRef module = LLVMModuleCreateWithName(moduleName);
    const char* Triple = "x86_64-pc-linux-gnu";
    LLVMSetTarget(module, Triple);

    // Generating print function 
    LLVMTypeRef printParam = LLVMInt32Type();
    LLVMTypeRef printParamTypes[] = {printParam};
    LLVMTypeRef voidRet = LLVMVoidType();
    LLVMTypeRef printFunc = LLVMFunctionType(voidRet, printParamTypes, 1, false);

    // Generating read function
    LLVMTypeRef readIntRet = LLVMInt32Type();
    LLVMTypeRef readFunc = LLVMFunctionType(readIntRet, NULL, 0, false);

    // Adding external functions to module
    const char* printFuncName = "print";
    LLVMValueRef printValRef = LLVMAddFunction(module, printFuncName, printFunc);
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

    // TESTING
    printTestSet(varNames);
    

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


    //printAllocMap(var_map);
    //printf("\n\n\n");

    // Add allocs to basic block?



    // Creating reference to return value allocation instruction

    // MAKE GLOBAL??
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
    LLVMValueRef loadRef = LLVMBuildLoad2(builder, LLVMInt32Type(), ret_ref, "load");
    LLVMValueRef retRef = LLVMBuildRet(builder, loadRef);
    LLVMInsertIntoBuilder(builder, loadRef);
    LLVMInsertIntoBuilder(builder, retRef);

    // Retrieving statement list
    astFunc func = (prog.func)->func;
    astStmt stmt = (func.body)->stmt;
    astBlock block = stmt.block;
    vector<astNode*>* statements = block.stmt_list;






    // Iterating through all statements in the node statement list
    for (auto stmtIter = statements->begin(); stmtIter != statements->end(); ++stmtIter) {
        astNode* statementNode = *stmtIter;
        LLVMBasicBlockRef exitBB = generateIRStatement(statementNode, builder, entryBB, var_map, readValRef, mainFuncRef);
        //printf("newentry: %s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(entryBB)));
        //printf("exit: %s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(exitBB)));
        fflush(stdout);
        entryBB = exitBB;


        // TESTING
        printf("newentry: %s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(entryBB)));
        printf("%s\n", LLVMPrintModuleToString(module));
        fflush(stdout);
    }
    








    /*
    
Start at the root (prog node):
Generate a module, set the target architecture
Generate LLVM functions without bodies for print and read extern function declarations
Visit the function node
Do the following for the function node:
Generate a LLVM builder.
Generate an entry basic block, and let entryBB be the reference to this basic block
Create a set with names of all parameters and local variables (strongly recommend using C++ strings for names).
Set the position of the builder to the end of entryBB
Initialize var_map to a new map: 
For each name in the set created above: 
Generate an alloc statement 
Add name, LLVMvalueRef of alloc statement generated above to var_map  
Generate an alloc instruction for the return value and keep the LLVMValueRef, ret_ref, of this instruction available everywhere in your program
Generate a store instruction to store the function parameter (use LLVMGetParam) into the memory location associated with (alloc instruction) the parameter name in the function ast node. 
Generate a return basic block and keep the LLVMBasicBlockRef, retBB, of this basic block available everywhere in your program
Set the position of the builder to the end of retBB
Add the following LLVM instructions to retBB:
A load instruction from the ret_ref.
A return instruction with operand as the load instruction created above.
Generate IR for the function body by calling genIRStmt subroutine given below: pass entryBB as a parameter to genIRStmt and let the exitBB be the return value of genIRStmt call.



Get the terminator instruction of exitBB basic block:
if the terminator is NULL (there is no return statement at the end of the function body)
Set the position of the builder to the end of exitBB.
Generate an unconditional branch to retBB.
Remove all basic blocks that do not have any predecessor basic blocks (think breadth-first search (BFS) !!!)
Time for memory cleanup (delete memory from maps, sets, builder)!*/

    char* moduleTest = LLVMPrintModuleToString(module);
    printf("%s", moduleTest);
    fflush(stdout);

    return module;

}























/***************** generateIRStatement ***********************/
LLVMBasicBlockRef generateIRStatement(astNode* statementNode, LLVMBuilderRef builder, 
        LLVMBasicBlockRef startBB, unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef,
        LLVMValueRef mainFuncRef) 
{
    // Extracting statement from statement node
    astStmt stmt = statementNode->stmt;



    // Testing
    //printf("Printing node\n");

    //printNode(statementNode);
    fflush(stdout);








    switch (stmt.type) {




    /*If the statement is an assignment statement:
Set the position of the builder to the end of startBB
Generate LLVMValueRef of RHS by calling the genIRExpr subroutine given below.
Generate an LLVM store instruction to store the RHS LLVMValueRef to the memory location (alloc instruction) corresponding to the LHS.
return startBB (startBB is also the endBB in case of an assignment statement) (?????)*/

        case ast_asgn: {

            // Testing
            // printf("Entered assgn\n");
            // fflush(stdout);

            // printf("basic block state3.1: %s", LLVMPrintValueToString(LLVMBasicBlockAsValue(startBB)));
            // fflush(stdout);





            // Positioning builder at end of start basic block
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generating LLVMValueRef of RHS of assignment statement
            astNode* rhsNode = stmt.asgn.rhs;


            // printf("basic block state3: %s", LLVMPrintValueToString(LLVMBasicBlockAsValue(startBB)));
            // fflush(stdout);




            LLVMValueRef rhs = generateIRExpression(rhsNode, builder, var_map, readValRef, startBB);





            // printf("ASDF: %s\n", LLVMPrintValueToString(rhs));
            // fflush(stdout);






            // Retrieving variable name from LHS of assignment statement
            astNode* lhsNode = stmt.asgn.lhs;
            string lhsName(lhsNode->var.name);
            LLVMValueRef newStoreInstRef = LLVMBuildStore(builder, rhs, var_map.at(lhsName));



            // Testing
            // printf("ASDF2: %s\n", LLVMPrintValueToString(newStoreInstRef));
            // fflush(stdout);





            // Inserting new instruction into basic block
            //LLVMInsertIntoBuilder(builder, newStoreInstRef);




            // Testing
            printf("%s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(startBB)));
            fflush(stdout);





            return startBB;
        }







            /*
If the statement is a call statement (it must be a call to print function as per the grammar ):
Set the position of the builder to the end of startBB
Generate LLVMValueRef of the value being printed by calling the genIRExpr subroutine given below.
Generate a Call instruction to the print function and use LLVMValueRef as parameter to the call.
return startBB (startBB is also the endBB in case of an assignment statement)

??????????

*/


        // case ast_call: {

        //     if (strcmp(statementNode->ext.name, "print") == 0) {

        //         // Setting position of builder to end of startBB
        //         LLVMPositionBuilderAtEnd(builder, startBB);

        //         statementNode->




        //     }



        // }



        /*If the statement is a while statement:
Set the position of the builder to the end of startBB.
Generate a basic block to check the condition of the while loop. Let this be condBB.
Generate an unconditional branch at the end of startBB to condBB.
Set the position of the builder to the end of condBB.
Generate LLVMValueRef of the relational expression (comparison) in the condition of the while loop by calling the genIRExpr subroutine given below.
Generate two basic blocks, trueBB and falseBB, that will be the successor basic blocks when condition is true or false respectively.
Generate a conditional branch at the end of condBB using the LLVMValueRef of comparison in step E above and setting the successor as trueBB when the condition is true and the successor as falseBB when the condition is false.






Generate the LLVM IR for the while loop body by calling the genIRStmt subroutine recursively: pass trueBB as a parameter to genIRStmt and let the trueExitBB be the return value of genIRStmt call.
Set the position of the builder to the end of trueExitBB.
Generate an unconditional branch to condBB at the end of trueExitBB.
Return falseBB as the endBB for a while statement.

*/
        case ast_while: {

            // Setting position of builder to the end of startBB
            LLVMPositionBuilderAtEnd(builder, startBB);

            // Generating basic block for condition of while loop
            
            char* newName = (char*) malloc(strlen(generateNewBBName(startBB) + 1));
            newName = strcpy(newName, generateNewBBName(startBB));
            
            
            //generateNewBBName(startBB);
            LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(mainFuncRef, newName);

            


            // Testing
            printf("bbname: %s\n", generateNewBBName(startBB));
            fflush(stdout);














            // Linking startBB to condBB w/ unconditional branch
            LLVMValueRef newUncondBrRef = LLVMBuildBr(builder, condBB);

            // Moving builder to end of condBB
            LLVMPositionBuilderAtEnd(builder, condBB);

            // Extract while condition node from while statement node
            astWhile whileStatement = statementNode->stmt.whilen;
            astNode* whileCondNode = whileStatement.cond;
            LLVMValueRef newRelExprRef = generateIRExpression(whileCondNode, builder, var_map, readValRef, startBB);

            // Generating true and false basic blocks
            char* newNameTrue = (char*) malloc(strlen(generateNewBBName(condBB) + 1));
            newNameTrue = strcpy(newNameTrue, generateNewBBName(condBB));
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(mainFuncRef, newNameTrue);

            char* newNameFalse = (char*) malloc(strlen(generateNewBBName(trueBB) + 1));
            newNameFalse = strcpy(newNameFalse, generateNewBBName(trueBB));
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(mainFuncRef, newNameFalse);


            LLVMBuildCondBr(builder, newRelExprRef, trueBB, falseBB);

            // Retrieve while loop body node
            astNode* whileBody = whileStatement.body;



            // Testing
            // printf("test1: %s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(startBB)));
            // fflush(stdout);
            // printf("test2: %s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(condBB)));
            // fflush(stdout);
            // printf("test3: %s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(trueBB)));
            // fflush(stdout);










// WORK POINT

            LLVMBasicBlockRef trueExitBB = generateIRStatement(whileBody, builder, trueBB, var_map, readValRef, mainFuncRef);



            //Testing
            // printf("ASDFASDFASDF\n");
            // fflush(stdout);






            // Setting position of builder to end of trueExitBB
            LLVMPositionBuilderAtEnd(builder, trueExitBB);

            // Generating unconditional branch linking trueExitBB to condBB
            LLVMBuildBr(builder, condBB);


            printf("%s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(trueExitBB)));
            fflush(stdout);

            return falseBB;











        }




        /*


If the statement is an if statement:
Set the position of the builder to the end of startBB.
Generate LLVMValueRef of the relational expression (comparison) in the condition of the if statement by calling the genIRExpr subroutine given below.
Generate two basic blocks, trueBB and falseBB, that will be the successor basic blocks when condition is true or false respectively.
Generate a conditional branch at the end of startBB using the LLVMValueRef of comparison in step B above, and setting the successor as trueBB when the condition is true and the successor as falseBB when the condition is false.

*/
    case ast_if: {

        // Setting position of builder to end of startBB
        LLVMPositionBuilderAtEnd(builder, startBB);

        // Extract if condition node from while statement node
        astIf ifStatement = statementNode->stmt.ifn;
        astNode* ifCondNode = ifStatement.cond;
        LLVMValueRef newRelExprRef = generateIRExpression(ifCondNode, builder, var_map, readValRef, startBB);

        // Generating true and false basic blocks
        LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(mainFuncRef, "");
        LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(mainFuncRef, "");

        


    /*


If there is no else part to the if statement
Generate the LLVM IR for the if body by calling the genIRStmt subroutine recursively: pass trueBB as a parameter to genIRStmt and let the ifExitBB be the return value of genIRStmt call.
Set the position of the builder to the end of ifExitBB.
Add an unconditional branch to falseBB.
Return falseBB as the endBB.
*/  


        // Retrieving else node from the if statement node
        astNode* elseNode = ifStatement.else_body; 

        
        if (elseNode == NULL) {
            astNode* ifNode = ifStatement.if_body;
            LLVMBasicBlockRef ifExitBB = generateIRStatement(ifNode, builder, trueBB, var_map, readValRef, mainFuncRef);

            // Positioning builder at end of ifExitBB
            LLVMPositionBuilderAtEnd(builder, ifExitBB);

            // Adding unconditional branch to falseBB
            LLVMBuildBr(builder, falseBB);

            return falseBB;
        }




        /*




If there is an else part to the if statement:
Generate the LLVM IR for the if body by calling the genIRStmt subroutine recursively: pass trueBB as a parameter to genIRStmt and let the ifExitBB be the return value of genIRStmt call.
Generate the LLVM IR for the else body by calling the genIRStmt subroutine recursively: pass falseBB as a parameter to genIRStmt and let the elseExitBB be the return value of genIRStmt call.
Generate a new endBB basic block.
Set the position of the builder to the end of ifExitBB.
Add an unconditional branch to endBB.
Set the position of the builder to the end of elseExitBB.
Add an unconditional branch to endBB.
return endBB.


*/
        else {

            // Generate instructions for statements in if-body and else-body
            astNode* ifNode = ifStatement.if_body;
            LLVMBasicBlockRef ifExitBB = generateIRStatement(ifNode, builder, trueBB, var_map, readValRef, mainFuncRef);
            LLVMBasicBlockRef elseExitBB = generateIRStatement(elseNode, builder, falseBB, var_map, readValRef, mainFuncRef);

            LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(mainFuncRef, "");

            // Linking trueBB and falseBB to common basic block
            LLVMPositionBuilderAtEnd(builder, ifExitBB);
            LLVMBuildBr(builder, endBB);
            LLVMPositionBuilderAtEnd(builder, elseExitBB);
            LLVMBuildBr(builder, endBB);

            return endBB;

        }
    }

    /*

If the statement is a return statement: 
Set the position of the builder to the end of startBB.
Generate LLVMValueRef of the return value (could be an expression) by calling the genIRExpr subroutine given below.
Generate a store instruction from LLVMValueRef of return value to the memory location pointed by ret_ref alloc statement.
Generate an unconditional branch to retBB.
Generate a new endBB basic block and return it


*/

// STARTING HERE
        case ast_ret: {

            printf("test: %s\n", LLVMPrintValueToString(LLVMBasicBlockAsValue(startBB)));
            fflush(stdout);

            


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
            char* newBlockName = (char*) malloc(strlen(generateNewBBName(startBB) + 1));
            newBlockName = strcpy(newBlockName, generateNewBBName(startBB));
            LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(mainFuncRef, newBlockName);

            printf("test2\n");
            fflush(stdout);

            return endBB;
        }



/*



If the statement is a block statement:
Set a variable prevBB to startBB
for each statement S in the statement list in the block statement:
Generate the LLVM IR for S by calling the genIRStmt subroutine recursively: pass prevBB as a parameter to genIRStmt and assign the return value of genIRStmt call to prevBB (this connects each statement to the next statement)
return the value returned by the call to genIRStmt on the last statement in the statement list as endBB.*/

        case ast_block: {
            LLVMBasicBlockRef prevBB = startBB;
            astBlock blockNode = statementNode->stmt.block;

            // printf("basic block state4: %s", LLVMPrintValueToString(LLVMBasicBlockAsValue(prevBB)));
            // fflush(stdout);


            // Retrieving list of statements from block
            vector<astNode*> *statementList = blockNode.stmt_list;

            // 
            for (auto statIter = statementList->begin(); statIter != statementList->end(); ++statIter) {

                astNode* statementNode = *statIter;
                astStmt statement = statementNode->stmt;

                // printf("basic block state5: %s", LLVMPrintValueToString(LLVMBasicBlockAsValue(prevBB)));
                // fflush(stdout);

                prevBB = generateIRStatement(statementNode, builder, prevBB, var_map, readValRef, mainFuncRef);
            }
            
            return prevBB; 
        }
    }































return startBB;



}














































/***************** generateIRExpression ***********************/
LLVMValueRef generateIRExpression(astNode* expressionNode, LLVMBuilderRef builder,
        unordered_map<string, LLVMValueRef> var_map, LLVMValueRef readValRef, LLVMBasicBlockRef bb)
{
    switch (expressionNode->type) {

/*    If the node is a constant:
Generate an LLVMValueRef using LLVMConstInt using the constant value in the node.
Return the LLVMValueRef of the LLVM Constant.
*/

        // Case 1: Constant node
        case ast_cnst: {

            // creating a new LLVM reference to constant value
            unsigned long long intValue = expressionNode->cnst.value;
            LLVMValueRef newConstInst = LLVMConstInt(LLVMInt32Type(), intValue, true);
            return newConstInst;
        }


//herehere













/*If the node is a var:
Generate a load instruction that loads from the memory location (alloc instruction in var_map) corresponding to the variable name in the node.
Return the LLVMValueRef of the load instruction.*/

        // Case 2: Var node
        case ast_var: {

            // Retrieving variable name from node
            char* varNameAsChar = expressionNode->var.name;
            string varName(varNameAsChar);

            // Retrieving variable value from memory and creating load instruction
            // varNameAsChar
            LLVMValueRef memLoc = var_map.at(varName);

            // memLoc != value in map
            //printf("testthree\n");
            //printf("testthree\n");

            // pointer val below may need to be different

            //printf("errortest: %s\n", LLVMPrintValueToString(memLoc));
            //fflush(stdout);

            // printf("basic block state: %s", LLVMPrintValueToString(LLVMBasicBlockAsValue(bb)));
            // fflush(stdout);

            

            // reposition builder?





            // i think issue is with builder
            LLVMValueRef newLoadInstRef = LLVMBuildLoad2(builder, LLVMInt32Type(), memLoc, "");

            // printf("errortest2: %s\n", LLVMPrintValueToString(newLoadInstRef));
            // fflush(stdout);

            // printf("testTwo\n");
            // fflush(stdout);
            
            return newLoadInstRef;

        }
















/*If the node is a unary expression:
Generate LLVMValueRef for the expr in the unary expression node by calling genIRExpr recursively.
Generate a subtraction instruction with constant 0 as the first operand and LLVMValueRef from step A above as the second operand.
Return the LLVMValueRef of the subtraction instruction.*/

        case ast_uexpr: {

            // Generating LLVM reference for inner expression node
            astNode* innerExpNode = expressionNode->uexpr.expr;
            LLVMValueRef innerExpNodeRef = generateIRExpression(innerExpNode, builder, var_map, readValRef, bb);

            unsigned long long zeroValue = 0;
            LLVMValueRef zeroValueRef = LLVMConstInt(LLVMInt32Type(), zeroValue, true);
            LLVMValueRef subInst = LLVMBuildSub(builder, zeroValueRef, innerExpNodeRef, "");



            
        }


/*If the node is a binary expression:
Generate LLVMValueRef for the lhs in the binary expression node by calling genIRExpr recursively.
Generate LLVMValueRef for the rhs in the binary expression node by calling genIRExpr recursively.
Based on the operator in the binary expression node, generate an addition/subtraction/multiplication instruction using LLVMValueRef of lhs and rhs as operands.
Return the LLVMValueRef of the instruction generated in step C.*/

//here
        case ast_bexpr: {

            // Generating LLVM reference for lhs inner expression node
            astNode* lhsInnerExpNode = expressionNode->bexpr.lhs;
            LLVMValueRef lhsInnerExpNodeRef = generateIRExpression(lhsInnerExpNode, builder, var_map, readValRef, bb);

            // printf("constanttest: %s\n", LLVMPrintValueToString(lhsInnerExpNodeRef));
            // fflush(stdout);

            // printf("basic block state2: %s", LLVMPrintValueToString(LLVMBasicBlockAsValue(bb)));
            // fflush(stdout);

            // Generating LLVM reference for rhs inner expression node
            astNode* rhsInnerExpNode = expressionNode->bexpr.rhs;
            LLVMValueRef rhsInnerExpNodeRef = generateIRExpression(rhsInnerExpNode, builder, var_map, readValRef, bb);

            op_type operation = expressionNode->bexpr.op;

            switch (operation) {
                case add: {
                    LLVMValueRef newAddInstRef = LLVMBuildAdd(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newAddInstRef;
                }
                
                case sub: {
                    LLVMValueRef newSubInstRef = LLVMBuildSub(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newSubInstRef;
                }

                case mul: {
                    LLVMValueRef newMulRef = LLVMBuildMul(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newMulRef;
                }

                case divide: {
                    LLVMValueRef newDivRef = LLVMBuildSDiv(builder, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newDivRef;
                }
            }
        }

        /*If the node is a relational (comparison) expression:
Generate LLVMValueRef for the lhs in the binary expression node by calling genIRExpr recursively.
Generate LLVMValueRef for the rhs in the binary expression node by calling genIRExpr recursively.
Based on the operator in the relational expression node, generate a compare instruction with parameter LLVMIntPredicate Op set to a comparison operator from LLVMIntPredicate enum in Core.h file.  The comparison operator should correspond to op in the given ast node.
Return the LLVMValueRef of the instruction generated in step C.*/


// WORK POINTL 

        case ast_rexpr: {

            // Generating LLVM reference for lhs inner expression node
            astNode* lhsInnerExpNode = expressionNode->rexpr.lhs;
            LLVMValueRef lhsInnerExpNodeRef = generateIRExpression(lhsInnerExpNode, builder, var_map, readValRef, bb);

            // Generating LLVM reference for rhs inner expression node
            astNode* rhsInnerExpNode = expressionNode->rexpr.rhs;
            LLVMValueRef rhsInnerExpNodeRef = generateIRExpression(rhsInnerExpNode, builder, var_map, readValRef, bb);

            rop_type operation = expressionNode->rexpr.op;
            LLVMIntPredicate operationType;
            LLVMValueRef newCmpInstRef;

            switch (operation) {
                case lt: {
                    operationType = LLVMIntSLT;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                }



                case gt: {
                    operationType = LLVMIntSLT;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                }

                case le: {
                    operationType = LLVMIntSLE;
                    newCmpInstRef = LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                }

                case ge: {
                    operationType = LLVMIntSGT;
                    newCmpInstRef =LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                }

                case eq: {
                    operationType = LLVMIntEQ;
                    newCmpInstRef =LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                }

                case neq: {
                    operationType = LLVMIntNE;
                    newCmpInstRef =LLVMBuildICmp(builder, operationType, lhsInnerExpNodeRef, rhsInnerExpNodeRef, "");
                    return newCmpInstRef;
                }
            }
        }






    /*
If the node is a call (it must be a read function call as per the grammar):
Generate a call instruction to read function.
Return the LLVMValueRef of the call instruction*/

        case ast_call: {
            LLVMValueRef newCallRef = LLVMBuildCall2(builder, LLVMInt32Type(), readValRef, NULL, 0, "");
            return newCallRef;
        }

    }
}






























    void printTestSet(set<string> names) {
        printf("Printing var names\n");
        for (auto iter = names.begin(); iter != names.end(); ++iter) {
            string name = *iter;

            cout << name << endl;
            
        }
    }


    void printAllocMap(unordered_map<string, LLVMValueRef> allocMap) {
        for (auto iter = allocMap.begin(); iter != allocMap.end(); ++iter) {
            string name = iter->first;
            string allocInst = LLVMPrintValueToString(iter->second);

            printf("test\n");
            fflush(stdout);

            cout << name + " " + allocInst << endl;
            fflush(stdout);
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




/***************** generateNewBBName ***********************/
const char* generateNewBBName(LLVMBasicBlockRef bb) {

    const char* parentBBName = LLVMGetBasicBlockName(bb);
    
    string parentBBNameAsString(parentBBName);

    // Extracting block number of parent basic block
    string parentBBNum = parentBBNameAsString.substr(2);

    int newParentBBNum = stoi(parentBBNum);
    newParentBBNum++;

    string newParentBBName = "BB" + to_string(newParentBBNum);

    return (newParentBBName.c_str());

}