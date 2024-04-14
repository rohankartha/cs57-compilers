/**
 * 
 * 
*/

#include <cstddef>
#include <stack>
#include <bool.h>
#include "ast/ast.h"
using namespace std;

extern astNode* root;

bool semanticAnalysis() 
{
    // Creating stack to hold symbol tables
    stack<vector<char*>> *stStack = new stack<vector<char*>>();






}

/***************** analyzeNode ***********************/
void analyzeNode(stack<vector<char*>> *stStack, astNode* node)
{
    // Parameter checks
    if (stStack == NULL || node == NULL) {
    }

    // If the node is of type statement
    if (node->stmt.type != NULL) {
        if (node->stmt.type == ast_block) {

        }
        else if (node->stmt.type == ast_decl) {

        }
    }

    // If the node is of type node
    else if (node->type != NULL) {
        if (node->type == ast_func) {

        }
        else if (node->type == ast_cnst) {

        }
    }

    // For any other type of node
    else {

    }


}