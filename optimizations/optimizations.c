/**
 * optimizations.c
 * 
 * 
 * 
*/


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
using namespace std;

#define MAX_OPERAND 3

/***************** global function declarations ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb);
bool constantFolding(LLVMValueRef function);


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

        // Checking if instruction with same opcode + operands already encountered
        int check = 0;

        if (!historyMap.empty() && historyMap.count(opcode) > 0) {
            auto it = historyMap.find(opcode);
            vector<LLVMValueRef> historyOperands = it->second;

            if (historyOperands == operands) {
                printf("Common subexpression\n");
                check = 1;
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
bool cleanDeadCode(LLVMValueRef function) 
{
    // what functions can never be removed: store, 

    // remove common subexpressions, old instructions from constant folding

}


/***************** constantFolding ***********************/
bool constantFolding(LLVMValueRef function) 
{
    // Iterating through all basic blocks in the function
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); 
            basicBlock;
            basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
        
        // Iterating through each instruction in the basic block
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); 
                instruction;
                instruction = LLVMGetNextInstruction(instruction)) {
            
            // Retrieve opcode for instruction
            LLVMOpcode opCode = LLVMGetInstructionOpcode(instruction);
            char* test;
            LLVMValueRef constant;

            // If instruction is add, subtract or multiply, replace with constant instruction
            switch (opCode) {
                case LLVMAdd:
                    constant = LLVMConstAdd(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    test = LLVMPrintValueToString(constant);
                    printf("ADD: %s\n", test);
                    break;
                case LLVMSub:
                    constant = LLVMConstSub(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    test = LLVMPrintValueToString(constant);
                    printf("Sub: %s\n", test);
                    break;
                case LLVMMul:
                    constant = LLVMConstMul(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    test = LLVMPrintValueToString(constant);
                    printf("Mult: %s\n", test);
                    break;
            }
        }
    }
}

// What LLVM instructions involve variables? 
// store is direct (storing in memory), load is indirect (loading from memory into register)
// all direct assignments are stores, not all stores are direct assignments
// store instructions for assignments, store instructions for operations


/***************** computeGen ***********************/
set<LLVMValueRef> computeGen(LLVMBasicBlockRef bb) 
{
    // Initializing gen set to empty set
    set<LLVMValueRef> genSet; 

    // Iterate over all instructions in basic block 
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
            instruction != NULL; 
            instruction = LLVMGetNextInstruction(instruction)) {
        
        // If instruction is a store, add it to set
        LLVMOpcode opCode = LLVMGetInstructionOpcode(instruction);
        if (opCode == LLVMStore) {

            // Check if instruction kills any other instructions within the basic block
            auto oldInst = genSet.begin();

            while (oldInst != genSet.end()) {
                LLVMValueRef instRef = *oldInst;
                LLVMOpcode instRefOpCode = LLVMGetInstructionOpcode(instRef);

                if (instRefOpCode == LLVMStore) {
                    LLVMValueRef memLoc = LLVMGetOperand(instRef, 1);

                    // Remove instruction that is killed
                    if (memLoc == LLVMGetOperand(instruction, 1)) {

                        // Does this erase a copy or the original???
                        genSet.erase(instRef);
                    }
                    ++oldInst;
                }
            }

            // Insert instruction into gen set
            genSet.insert(instruction);
        }
        // what if three store to same location

    }
    return genSet;
}


/***************** computeKill ***********************/
set<LLVMValueRef> computeKill(LLVMBasicBlockRef bb) {

    // Create set of all store instructions in function

    // Initializing kill set to empty set

    // Iterate over all instructions in basic block 


}

/***************** computeInandOut ***********************/
vector<set<LLVMValueRef>> computeInandOut(LLVMBasicBlockRef bb) {}






/***************** constantPropagation ***********************/
bool constantPropagation(unordered_map<LLVMBasicBlockRef, LLVMValueRef> basicBlockSets) {}
