#include <stdlib.h>
#include <stdbool.h>
#include "Core.h"

#define MAX_OPERAND 3


typedef struct tracker {
    LLVMOpcode opcode;
    int operand[MAX_OPERAND]; // may have to intiialize to 0
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

    // Iterating through basic block instructions
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
        instruction != NULL; 
        instruction = LLVMGetNextInstruction) {

        LLVMOpcode opCode = LLVMGetInstructionOpcode(instruction);
        


}


/***************** getBasicBlockSize ***********************/
int getBasicBlockSize(LLVMBasicBlockRef bb)
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