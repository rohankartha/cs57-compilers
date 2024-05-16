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
#include "../front-end/semantic-analysis.h"
#include <iostream>
using namespace std;

/***************** local-global function declarations ***********************/
unordered_map<string, LLVMValueRef> renameVariables();
void addVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef>* varNameMap, unordered_map<string, int>* nameFreqMap);
static void printTest(unordered_map<string, LLVMValueRef>* varNameMap);
static void printTestTwo(unordered_map<string, int>* nameFreqMap);


/***************** renameVariables ***********************/
unordered_map<string, LLVMValueRef> renameVariables() 
{
    // Initializing maps to store variables, alloc instructions
    unordered_map<string, LLVMValueRef>* varNameMap = new unordered_map<string, LLVMValueRef>;
    int check = 0;
    unordered_map<string, int>* nameFreqMap = new unordered_map<string, int>;

    // Extracting block statement node from root node
    astProg prog = root->prog;
    astFunc func = (prog.func)->func;
    astStmt stmt = (func.body)->stmt;
    astBlock block = stmt.block;


    vector<astNode*> *statementList = block.stmt_list;
    // printf("test2: %d", statementList->size());
    // fflush(stdout);

    // Visit each statement node
    // for (int i = 0; i != statementList->size(); ++i) {
    //     astNode* statement = statementList->at(i);
    // }


    // astNode* asdf = statements->at(0);

    //why can't iterate through vector?

    for (auto statIter = statementList->begin(); statIter != statementList->end(); ++statIter) {

        astNode* statementNode = *statIter;
        astStmt statement = statementNode->stmt;
        //printf("ASDF");


        // types of statements that may have variables: call, ret, while, if, decl, assign
        switch (statement.type) {

            // param=NULL in ast.h?
            // case ast_call:
            //     astCall call = statement.call;
            //     if (call.param != NULL) {

            //     }
            //     break;

            // If statement is a return statement
            case ast_ret:
                astRet ret = statement.ret;
                astNode expr = *ret.expr;
                if (expr.type == ast_bexpr) {

                    astBExpr binaryexpr = expr.bexpr;

                    // If first term in expression is variable
                    if ((binaryexpr.lhs)->type == ast_var) {
                        astNode* lhs = binaryexpr.lhs;
                        astVar varOne = lhs->var;
                        string varName(varOne.name);
                        addVariable(varName, varNameMap, nameFreqMap);
                    }

                    // If second term in expression is variable
                    if ((binaryexpr.rhs)->type == ast_var) {
                        astNode* rhs = binaryexpr.rhs;
                        astVar varTwo = rhs->var;
                        string varName(varTwo.name);
                        addVariable(varName, varNameMap, nameFreqMap);
                    }

                    printTest(varNameMap);
                    printTestTwo(nameFreqMap);
                }
                break;





            // case ast_while:
            //     break;


            // If statement is an if-else statement
            // case ast_if:
            //     astIf ifstmt = statement.ifn;
            //     astRExpr condition = ifstmt.cond->rexpr;

            //     if ((condition.lhs)->type == ast_var) {
            //             astNode* lhs = condition.lhs;
            //             astVar varOne = lhs->var;
            //             string varName(varOne.name);
            //             addVariable(varName, varNameMap, nameFreqMap);
            //         }


                
            //     break;




            // case ast_decl:
            //     break;
            // case ast_asgn:
            //     break;
        }
    }
    
}


/***************** createTemporaryVariable ***********************/
void addVariable(string oldVarName, unordered_map<string, 
        LLVMValueRef>* varNameMap, unordered_map<string, int>* nameFreqMap)
{

    // check == 0 && 
    // REMOVE CHECK

    if (varNameMap->empty()) {
        varNameMap->insert({oldVarName, NULL});
        nameFreqMap->insert({oldVarName, 0});
        return;
    }

    else {
        auto iter = varNameMap->find(oldVarName);
        if (iter != varNameMap->end()) {
            string newVarName(oldVarName);
            
            // Constructing temporary variable name and adding it to the map
            auto nameIter = nameFreqMap->find(oldVarName);
            if (nameIter != nameFreqMap->end()) {

                int count = nameIter->second;
                count = count + 1;
                nameIter->second = count;

                newVarName = newVarName.append(to_string(count));
                varNameMap->insert({newVarName, NULL});
            }
            return;
        }
        else {
            varNameMap->insert({oldVarName, NULL});
            nameFreqMap->insert({oldVarName, 0});
            return;
        }
    }
}






static void printTest(unordered_map<string, LLVMValueRef>* varNameMap) {

    for (auto iter = varNameMap->begin(); iter != varNameMap->end(); ++iter) {
        string test = iter->first;


        cout << test << endl;
    }

}


static void printTestTwo(unordered_map<string, int>* nameFreqMap) {

    for (auto iter = nameFreqMap->begin(); iter != nameFreqMap->end(); ++iter) {
        string test = iter->first;
        int freq = iter->second;


        cout << test << endl;
        cout << freq << endl;
    }

}




