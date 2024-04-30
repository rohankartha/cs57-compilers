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
#include <algorithm>
using namespace std;

#define MAX_OPERAND 3

/***************** global type declarations ***********************/

typedef struct basicBlockSets {
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> genSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> killSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> inSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> outSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMBasicBlockRef>> predecessors;
} basicBlockSets_t;


/***************** global function declarations ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb);
bool constantFolding(LLVMValueRef function);
basicBlockSets_t computePredecessors(LLVMValueRef function);
set<LLVMValueRef> computeGen(LLVMBasicBlockRef bb);
set<LLVMValueRef> computeKill(LLVMBasicBlockRef bb, set<LLVMValueRef> storeSet);
basicBlockSets_t computeInandOut(LLVMValueRef function, basicBlockSets_t bbSets);





/***************** removeCommonSubexpression ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb) 
{
    // Validating parameters
    if (bb == NULL) {
        return false;
    }

    // Initializing vector to store instructions already encountered
    vector<LLVMValueRef> oldInstructions;
    int breakHelper = 0;

    // Iterating through basic block instructions
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
            instruction != NULL; 
            instruction = LLVMGetNextInstruction(instruction)) {

        char* instructionString = LLVMPrintValueToString(instruction);
        printf("candidate: %s\n", instructionString);

        // Retrieving opcode of instruction
        LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);

        // Iterate through set of instructions already encountered
        for (auto iter = oldInstructions.begin(); iter != oldInstructions.end(); ++iter) {
            LLVMValueRef oldInst = *iter;

            // Extract opcode from old instruction
            LLVMOpcode oldOpCode = LLVMGetInstructionOpcode(oldInst);

            // Check if opcode of old instruction matches that of current instruction
            if (oldOpCode == opcode) {

                // Extracting operands of new instruction
                vector<LLVMValueRef> newOperands;
                int newNumOperands = LLVMGetNumOperands(instruction);

                for (int i = 0; i < newNumOperands; i++) {
                    LLVMValueRef newOperand = LLVMGetOperand(instruction, i);
                    char* op = LLVMPrintValueToString(newOperand);
                    //printf("current Operand %d: %s \n", i, op);
                    newOperands.push_back(newOperand);
                }

                // Extracting operands of old instruction
                vector<LLVMValueRef> oldOperands;
                int oldNumOperands = LLVMGetNumOperands(oldInst);

                for (int i = 0; i < oldNumOperands; i++) {
                    LLVMValueRef oldOperand = LLVMGetOperand(oldInst, i);
                    char* op = LLVMPrintValueToString(oldOperand);
                    //printf("old Operand %d: %s\n", i, op);
                    oldOperands.push_back(oldOperand);
                }

                // If operands are same, move to next instruction in basic block
                if (newOperands == oldOperands) {

                    // If instruction is a load instruction, check for stores

                    char* test = LLVMPrintValueToString(instruction);
                    char* testTwo = LLVMPrintValueToString(oldInst);
                    printf("Common subexpression DETECTED\n");
                    printf("old: %s\n\n", testTwo);

                    // ADD STUFF HERE
                    newOperands.clear();
                    oldOperands.clear();
                    breakHelper = 1;
                    break;
                }
                newOperands.clear();
                oldOperands.clear();
            }
        }
        if (breakHelper == 0) {
            char* test = LLVMPrintValueToString(instruction);
            printf("Common subexpression NOT DETECTED\n\n");
            oldInstructions.push_back(instruction);
        }

        breakHelper = 0;
    }

        // // Clearing operand vector
        // operands.clear();
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
set<LLVMValueRef> computeKill(LLVMBasicBlockRef bb, set<LLVMValueRef> storeSet) 
{
    // Create set of all store instructions in function

    // Initializing kill set to empty set
    set<LLVMValueRef> killSet; 

    // Iterate over all instructions in basic block
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
            instruction != NULL; 
            instruction = LLVMGetNextInstruction(instruction)) {
    }


}

/***************** computeInandOut ***********************/
// basicBlockSets_t computeInandOut(LLVMValueRef function, basicBlockSets_t bbSets) 
// {
//     // Determine predecessors of each basic block
//     bbSets = computePredecessors(function);


//     // Iterating through basic blocks in function
//     for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); function != NULL;
//             bb = LLVMGetNextBasicBlock(bb)) {
        
//         //
//         // LLVMGetprevbasicblock
//         bbSets.predecessors.at(0);

//         outSetUnion(bbSets.predecessors.at(0), bbSets.predecessors.at(0));






//         // Calculate "in" set from union of predecessor "out" sets
//         // for (auto it = bbSets.predecessors.begin(); it != bbSets.predecessors.end(); ++it) {
//         //     LLVMBasicBlockRef predecessorBlock = *it;

//         //     set<LLVMValueRef> test = bbSets.outSets.find(predecessorBlock);

//         //     set_union()


//         // }



        


//     }






//     // Initialize "in" set 
//     set<LLVMValueRef> inSet;

//     // Setting "out" set to "gen" set
//     set<LLVMValueRef> outSet = genSet;

//     bool change = true;

    

//     while (change) {

//     }




// }





/***************** computePredecessors ***********************/
basicBlockSets_t computePredecessors(LLVMValueRef function, basicBlockSets_t bbSets) 
{

    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			basicBlock;
  			basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
        //

        LLVMBasicBlockRef predecessor = NULL;

        set<LLVMBasicBlockRef> bbPredecessors;

        while((predecessor = LLVMGetPreviousBasicBlock(basicBlock)) != NULL) {
            bbPredecessors.insert(predecessor);
        }

        bbSets.predecessors.insert(make_pair(basicBlock, bbPredecessors));
    }

    return bbSets;

}




/***************** union ***********************/
set<LLVMValueRef> outSetUnion(set<LLVMValueRef> setOne, set<LLVMValueRef> setTwo) 
{
    // Return the non-empty set if one of the sets is empty
    if (setOne.empty()) {
        return setTwo;
    }

    if (setTwo.empty()) {
        return setOne;
    }

    // Determine relative sizes of sets
    set<LLVMValueRef> smaller;
    set<LLVMValueRef> larger;

    if (setOne.size() > setTwo.size()) {
        smaller = setTwo;
        larger = setOne;
    }
    else {
        smaller = setOne;
        larger = setTwo;
    }

    set<LLVMValueRef> out_Union;

    // Iterate through smaller set
    for (auto it = smaller.begin(); it != smaller.end(); ++it) {

        LLVMValueRef instruction = *it;

        if (larger.find(instruction) != larger.end()) {
            out_Union.insert(instruction);
        }
    }
    
    return out_Union;
}


/***************** constantPropagation ***********************/
bool constantPropagation(unordered_map<LLVMBasicBlockRef, LLVMValueRef> basicBlockSets) {}
