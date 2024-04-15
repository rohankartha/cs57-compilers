#include <cstddef>
#include <stack>
#include "ast/ast.h"
#include <cstdio>
using namespace std;

extern astNode* root;
void semanticAnalysis();
bool analyzeNode(stack<vector<char*>> *stStack, ast_Node* node);
void analyzeFuncNode(stack<vector<char*>> *stStack, astFunc func);
bool analyzeVarNode(stack<vector<char*>> *stStack, astVar variable);
bool analyzeStmtNode(stack<vector<char*>> *stStack, astStmt stmt);
void analyzeOtherNode(stack<vector<char*>> *stStack, astNode* node);