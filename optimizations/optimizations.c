#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
using namespace std;

#define MAX_OPERAND 3

/***************** global function declarations ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb);


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

    // Initializing vectors to store instructions already encountered
    unordered_map<LLVMOpcode, vector<LLVMValueRef>> historyMap;

    // Iterating through basic block instructions
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
        instruction != NULL; 
        instruction = LLVMGetNextInstruction(instruction)) {

        // Retrieving opcode and number operands of instruction
        LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
        int numOperands = LLVMGetNumOperands(instruction);

        vector<LLVMValueRef> operands;

        printf("numOperands: %d\n", numOperands);

        // Retrieving operands from the instruction
        for (int i = 0; i < numOperands; i++) {
            LLVMValueRef newOperand = LLVMGetOperand(instruction, i);
            printf("Operand %d: \n", i);
            operands.push_back(newOperand);
        }

        int check = 0;

        // Checking if instruction with same opcode + operands already encountered
        if (!historyMap.empty() && historyMap.count(opcode) > 0) {
            auto it = historyMap.find(opcode);
            vector<LLVMValueRef> historyOperands = it->second;

            if (historyOperands == operands) {
                printf("Common subexpression\n");
                check = 1;

                // printf("SIZE: %d", operands.size());

                // for (int i = 0; i < operands.size(); i++) {
                //     LLVMValueRef testOp = operands.at(i);
                //     char* test = LLVMPrintValueToString(testOp);
                //     printf("ASDF%s\n\n", test);

                // }






            }
        }
        
        // If instruction with same opcode + operands not encountered, add it to map
        if (check == 0) {
            historyMap.insert({opcode, operands});
        }


        char* test = LLVMPrintValueToString(instruction);
        printf("%s\n\n", test);

        operands.clear();

    }
}


/***************** cleanDeadCode ***********************/
bool cleanDeadCode(LLVMValueRef function) {}

/***************** constantFolding ***********************/
bool constantFolding(LLVMValueRef function) {}