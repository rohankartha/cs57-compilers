#include <cstddef>
#include <stack>
#include "ast/ast.h"
#include <cstdio>
using namespace std;

extern astNode* root;
void semanticAnalysis();
bool analyzeNode(stack<vector<char*>> *stStack, ast_Node* node);
void analyzeFuncNode(stack<vector<char*>> *stStack, astFunc func);
void analyzeCnstNode(stack<vector<char*>> *stStack, astConst cnst);
void analyzeExternNode(stack<vector<char*>> *stStack, astExtern ext);
void analyzeStmtNode(stack<vector<char*>> *stStack, astStmt stmt);