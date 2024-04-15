#include <cstddef>
#include <stack>
#include "ast/ast.h"
#include <cstdio>
#include <cstring>
using namespace std;

extern astNode* root;
bool semanticAnalysis();
bool analyzeNode(stack<vector<char*>> *stStack, ast_Node* node);
bool analyzeFuncNode(stack<vector<char*>> *stStack, astFunc func);
bool analyzeVarNode(stack<vector<char*>> *stStack, astVar variable);
bool analyzeStmtNode(stack<vector<char*>> *stStack, astStmt stmt, int code);
bool analyzeOtherNode(stack<vector<char*>> *stStack, astNode* node);