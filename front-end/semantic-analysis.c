/**
 * semantic-analysis.c – Semantic analyzer for compiler. From the abstract syntax tree,
 * the analyzer checks two conditions: (1) a variable is declared before it is used; (2)
 * here is only one declaration of the variable in a scope
 * 
 * Rohan Kartha, April 2024
 * 
*/

#include <cstddef>
#include <stack>
#include "ast/ast.h"
#include "semantic-analysis.h"
#include <cstdio>
#include <cstring>
using namespace std;

/**************** global variables ****************/
astNode* root = NULL;

/**************** function prototypes ****************/
bool semanticAnalysis();
bool analyzeNode(stack<vector<char*>> *stStack, ast_Node* node);
bool analyzeFuncNode(stack<vector<char*>> *stStack, astFunc func);
bool analyzeVarNode(stack<vector<char*>> *stStack, astVar variable);
bool analyzeStmtNode(stack<vector<char*>> *stStack, astStmt stmt, int code);
bool analyzeOtherNode(stack<vector<char*>> *stStack, astNode* node);

// Assumptions: root node is program node


/**************** semanticAnalysis ****************/
bool semanticAnalysis() 
{
    // Creating stack to hold symbol tables
    stack<vector<char*>> *stStack = new stack<vector<char*>>();

    // Starting traversal at AST root
    bool semanticAnalysisResult = analyzeNode(stStack, root);

    // Freeing stack and returning result
    delete stStack;
    return semanticAnalysisResult;
}

/***************** analyzeNode ***********************/
bool analyzeNode(stack<vector<char*>> *stStack, ast_Node* node)
{
    bool result;

    switch(node->type) {
        
        case ast_prog:
            result = analyzeNode(stStack, root->prog.func);
            if (!result) { return false; }
            break;

        // If node is statement node
        case ast_stmt:
            result = analyzeStmtNode(stStack, node->stmt, 1);
            if (!result) { return false; }
            break;
        
        // If node is function node
        case ast_func:
            result = analyzeFuncNode(stStack, node->func);
            if (!result) { return false; }
            break;

        // If node is variable node
        case ast_var:
            result = analyzeVarNode(stStack, node->var);
            if (!result) { return false; }
            break;
        
        // If node another type
        default:
            result = analyzeOtherNode(stStack, node);
            if (!result) { return false; }
            break;
    }
    return true;
}


/***************** analyzeFuncNode ***********************/
bool analyzeFuncNode(stack<vector<char*>> *stStack, astFunc func) 
{

    // Creating new symbol table and pushing it onto stack
    vector<char*> curr_sym_table;
    stStack->push(curr_sym_table);

    // Adding parameter to symbol table if applicable
    astNode* node;
    if ((node = func.param) != NULL) {
        char* name = ((func.param)->var).name;
        curr_sym_table.push_back(name);           
    }

    // Updating copy of table on the stack
    if (!stStack->empty()) {
        stStack->pop();
    }
    
    stStack->push(curr_sym_table);

    // Extracting statements from function body
    astNode* body = func.body;
    astStmt stmt = body->stmt;

    // Visiting body of the function node
    bool result = analyzeStmtNode(stStack, stmt, 0);
    if (!result) { return false; }

    // Pop top of stack
    if (!stStack->empty()) {
        stStack->pop();
    }
    return true;
}


/***************** analyzeVarNode ***********************/
bool analyzeVarNode(stack<vector<char*>> *stStack, astVar variable) 
{
    // Get name of variable
    char* varName = variable.name;

    // Check if variable appears in a symbol table on a copy of the stack
    stack<vector<char*>> copyStack(*stStack);
    vector<char*> copySymTable;

    // Look through symbol tables in stack for variable
    while (!copyStack.empty()) {

        // Getting symbol table at the top of the copy stack
        copySymTable = copyStack.top();

        for (int i = 0; i < copySymTable.size(); ++i) {
            char* name = copySymTable.at(i);

            // Return true if the variable is found in a symbol table
            if (strcmp(varName, name) == 0) {
                return true;
            }
        }
        // Removing symbol table from copy stack
        if (!copyStack.empty()) {
            copyStack.pop();
        }
    }
    // If variable is not found, throw an error
    fprintf(stderr, "ERROR: variable '%s' has not been declared\n", varName);
    return false;
}


/***************** analyzeStmtNode ***********************/
bool analyzeStmtNode(stack<vector<char*>> *stStack, astStmt stmt, int code) 
{
    // Case 1: the statement is a block statement
    if (stmt.type == ast_block) {

        // Creating new symbol table and pushing it onto stack if block is not the body of a function
        if (code == 1) {
            vector<char*> curr_sym_table;
            stStack->push(curr_sym_table);
        }

        // Get list of statements which make up the block
        astBlock block = stmt.block;
        vector<astNode*> *statementList = block.stmt_list;

        // Visit each statement node
        for (int i = 0; i != statementList->size(); ++i) {
            astNode* statement = statementList->at(i);
            bool result = analyzeNode(stStack, statement);

            if (!result) { return false; }
        }

        // Popping symbol table off of the stack
        if (!stStack->empty() && code == 1) {
            stStack->pop();
        }
    }
    
    // Case 2: the statement is a declaration statement
    else if (stmt.type == ast_decl) {

        // Extract the name of the new variable from the declaration
        astDecl declaration = stmt.decl;
        char* variableName = declaration.name;
        vector<char*> curr_sym_table;

        // Checking if variable is in the symbol table at the top of the stack
        if (!stStack->empty()) {
            curr_sym_table = stStack->top();

            for (int i = 0; i < curr_sym_table.size(); i++) {
                char* name = curr_sym_table.at(i);

                // Throw error if variable is already in the table (i.e. it has already been declared)
                if (strcmp(name, variableName) == 0) {
                    printf("\nERROR: %s has already been declared in this scope\n", variableName);
                    return false; 
                }
            }
        }
        // Otherwise, add symbol at the top of the stack
        curr_sym_table.push_back(variableName);

        // Replace old symbol table on stack with updated version
        if (!stStack->empty()) {
            stStack->pop();
        }
        stStack->push(curr_sym_table);
    }

    // For all other statement node types, visit children nodes

    // Case 3: the statement is a call statement
    else if (stmt.type == ast_call) {
        astCall call = stmt.call;

        if (call.param != NULL) {
            bool result = analyzeNode(stStack, call.param);
            if (!result) { return false; }
        }
    }

    // Case 4: the statement is a return statement
    else if (stmt.type == ast_ret) {
        astRet ret = stmt.ret;
        bool result = analyzeNode(stStack, ret.expr);
        if (!result) { return false; }
    }

    // Case 5: the statement is a while statement
    else if (stmt.type == ast_while) {
        astWhile whileStmt = stmt.whilen;
        bool result = analyzeNode(stStack, whileStmt.cond);
        if (!result) { return false; }
    }

    // Case 6: the statement is an if statement
    else if (stmt.type == ast_if) {
        astIf ifStmt = stmt.ifn;
        bool result;

        result = analyzeNode(stStack, ifStmt.cond);
        if (!result) { return false; }

        result = analyzeNode(stStack, ifStmt.if_body);
        if (!result) { return false; }

        if (ifStmt.else_body != NULL) {
            result = analyzeNode(stStack, ifStmt.else_body);
            if (!result) { return false; }
        }

        return true;
    }

    // Case 7: the statement is an assign statement
    else if (stmt.type == ast_asgn) {
        astAsgn asgn = stmt.asgn;
        bool result;

        result = analyzeNode(stStack, asgn.lhs);
        if (!result) { return false; }

        result = analyzeNode(stStack, asgn.rhs);
        if (!result) { return false; }
    }
    return true;
}


/***************** analyzeOtherNode ***********************/
bool analyzeOtherNode(stack<vector<char*>> *stStack, astNode* node) 
{
    bool result; 

    switch (node->type) {

        // Visiting children nodes of comparison expression node
        case ast_rexpr:
            result = analyzeNode(stStack, node->rexpr.lhs);
            if (!result) { return false; }

            result = analyzeNode(stStack, node->rexpr.rhs);
            if (!result) { return false; }

            break;

        // Visiting children nodes of binary expression node
        case ast_bexpr:
            result = analyzeNode(stStack, node->bexpr.lhs);
            if (!result) { return false; }
            
            result = analyzeNode(stStack, node->bexpr.rhs);
            if (!result) { return false; }

            break;

        // Visiting children nodes of unary expression node
        case ast_uexpr:
            result = analyzeNode(stStack, node->uexpr.expr);
            if (!result) { return false; }
            break;
    }
    return true;
}