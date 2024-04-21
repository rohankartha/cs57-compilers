#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>

#define MAX_OPERAND 3

/***************** global function declarations ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb);

/***************** local function declarations ***********************/
static int getBasicBlockSize(LLVMBasicBlockRef bb);


/***************** local type declarations ***********************/
typedef struct tracker {
    LLVMOpcode opcode;
    int operand[MAX_OPERAND]; // may have to initialize to 0
} tracker_t;


/***************** removeCommonSubexpression ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb) 
{
    // Validating parameters
    if (bb == NULL) {
        return false;
    }

    // Calculating number of instructions in block and initializing tracker
    int numInstructions = getBasicBlockSize(bb);
    tracker_t* instructionTracker[numInstructions];
    int counter = 0;

    // Iterating through basic block instructions
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
        instruction != NULL; 
        instruction = LLVMGetNextInstruction) {

        instructionTracker[counter]->opcode = LLVMGetInstructionOpcode(instruction);
        
        printf("%d", instructionTracker[counter]->opcode);
    }
}


/***************** getBasicBlockSize ***********************/
static int getBasicBlockSize(LLVMBasicBlockRef bb)
{
    // Validating parameters
    if (bb == NULL) {
        return false;
    }

    int count = 0;

    // Counting number of instructions in the basic block
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
        instruction != NULL; 
        instruction = LLVMGetNextInstruction) {
            count++;
    }
    return count;
}






/***************** cleanDeadCode ***********************/
bool cleanDeadCode(LLVMValueRef function) {}

/***************** constantFolding ***********************/
bool constantFolding(LLVMValueRef function) {}