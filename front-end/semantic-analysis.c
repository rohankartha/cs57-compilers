/**
 * 
 * 
*/

#include <cstddef>
#include <stack>
#include "ast/ast.h"
#include <cstdio>
#include <cstring>
using namespace std;

extern astNode* root;
void semanticAnalysis();
bool analyzeNode(stack<vector<char*>> *stStack, ast_Node* node);
void analyzeFuncNode(stack<vector<char*>> *stStack, astFunc func);
bool analyzeVarNode(stack<vector<char*>> *stStack, astVar variable);
bool analyzeStmtNode(stack<vector<char*>> *stStack, astStmt stmt);
void analyzeOtherNode(stack<vector<char*>> *stStack, astNode* node);

// Assumptions: root node is program node
void semanticAnalysis() 
{
    // Creating stack to hold symbol tables
    stack<vector<char*>> *stStack = new stack<vector<char*>>();

    // Starting traversal at AST root
    analyzeNode(stStack, root);
}



// each node has a type, using node access member of union, from member of union access children. think of prog like glasses


/***************** analyzeNode ***********************/
bool analyzeNode(stack<vector<char*>> *stStack, ast_Node* node)
{
    // Parameter checks
    if (stStack == NULL || node == NULL) {
        return false;
    }

    switch(node->type) {
        
        case ast_prog:
            analyzeNode(stStack, root->prog.func);
            return true;
            break;

        // If node is statement node
        case ast_stmt:
            analyzeStmtNode(stStack, node->stmt);
            break;
        
        // If node is function node
        case ast_func:
            analyzeFuncNode(stStack, node->func);
            break;

        // If node is constant node
        case ast_var:
            printf("ASDFASDFASD");
            analyzeVarNode(stStack, node->var);
            break;
        
        // If node another type
        default:
            analyzeOtherNode(stStack, node);
            break;
    }
}


/***************** analyzeFuncNode ***********************/
void analyzeFuncNode(stack<vector<char*>> *stStack, astFunc func) 
{
    printf("func\n");

    // Creating new symbol table and pushing it onto stack
    vector<char*> *curr_sym_table = new vector<char*>();
    stStack->push(*curr_sym_table);

    // Adding parameter to symbol table if applicable
    astNode* node;
    if ((node = func.param) != NULL) {
        char* name = ((func.param)->var).name;
        curr_sym_table->push_back(name);           
    }

    // Extracting statements from function body
    astNode* body = func.body;
    astStmt stmt = body->stmt;

    // Visiting body of the function node
    analyzeStmtNode(stStack, stmt);

    // Pop top of stack
    stStack->pop();
}


/***************** analyzeVarNode ***********************/
bool analyzeVarNode(stack<vector<char*>> *stStack, astVar variable) 
{
    printf("var\n");
    // Get name of variable
    char* varName = variable.name;

    // Check if variable appears in a symbol table on a copy of the stack
    stack<vector<char*>> copyStack(*stStack);
    vector<char*> copySymTable;

    while (!copyStack.top().empty()) {

        // Getting symbol table at the top of the copy stack
        copySymTable = copyStack.top();

        for (int i = 0; i < copySymTable.size(); i++) {
            char* name = copySymTable.at(i);

            // Return true if the variable is found in a symbol table
            if (strcmp(varName, name) == 0) {
                return true;
            }
            // Removing symbol table from copy stack
            copyStack.pop();
        }
    }
    // If false, throw an error
    return false;

    // EMIT ERROR WITH NAME OF VARIABLE
}


/***************** analyzeStmtNode ***********************/
bool analyzeStmtNode(stack<vector<char*>> *stStack, astStmt stmt) 
{
    printf("stmt\n");

    if (stmt.type == ast_block) {

        // Creating new symbol table and pushing it onto stack
        vector<char*> *curr_sym_table = new vector<char*>();
        stStack->push(*curr_sym_table);

        // Visiting all statement nodes in the block
        astBlock block = stmt.block;
        vector<astNode*> *statementList = block.stmt_list;
        for (int i = 0; i != statementList->size(); ++i) {
            astNode* statement = statementList->at(i);
            analyzeNode(stStack, statement);
        }

        // Popping symbol table off of the stack
        stStack->pop();
    }
    
    else if (stmt.type == ast_decl) {
        astDecl declaration = stmt.decl;
        char* variableName = declaration.name;
        vector<char*> curr_sym_table;

        // Checking if variable is in the symbol table at the top of the stack
        if (!stStack->top().empty()) {
            curr_sym_table = stStack->top();

            for (int i = 0; i < curr_sym_table.size(); i++) {
                char* name = curr_sym_table.at(i);

                // Throw error if true
                if (strcmp(name, variableName) == 0) {
                    return true; 
                }
            }
        }

        // Otherwise, add symbol at the top of the stack
        curr_sym_table.push_back(variableName);
    }
}


/***************** analyzeOtherNode ***********************/
void analyzeOtherNode(stack<vector<char*>> *stStack, astNode* node) 
{
    printf("other\n");
    switch (node->type) {
        case ast_rexpr:
            analyzeNode(stStack, node->rexpr.lhs);
            analyzeNode(stStack, node->rexpr.rhs);
            break;

        // Visiting children nodes of binary expression node
        case ast_bexpr:
            analyzeNode(stStack, node->bexpr.lhs);
            analyzeNode(stStack, node->bexpr.rhs);
            break;

        // Visiting children nodes of unary expression node
        case ast_uexpr:
            analyzeNode(stStack, node->uexpr.expr);
            break;
    }
}