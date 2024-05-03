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
#include "optimizations.h"
using namespace std;

#define MAX_OPERAND 3

/***************** global type declarations ***********************/
typedef struct basicBlockSets {
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> genSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> killSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> inSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> outSets;
    unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predecessors;
} basicBlockSets_t;

/***************** global function declarations ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb);
bool constantFolding(LLVMValueRef function);
set<LLVMValueRef> computeGen(LLVMBasicBlockRef bb);
set<LLVMValueRef> computeKill(LLVMBasicBlockRef bb, set<LLVMValueRef> storeSet);
vector<set<LLVMValueRef>> computeInandOut(LLVMValueRef function, basicBlockSets_t completeSets);
bool constantPropagation(LLVMValueRef function);
void printSet(set<LLVMValueRef> testSet);
void printVector(unordered_map<LLVMValueRef, vector<LLVMValueRef>> allStoreSet);
unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> computePredecessors(LLVMValueRef function);
void printVectorTwo(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> allStoreSet);
set<LLVMValueRef> findSetUnion(set<LLVMValueRef> setOne, set<LLVMValueRef> setTwo);
set<LLVMValueRef> setMinus(set<LLVMValueRef> inSet, set<LLVMValueRef> killSet);




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

        // TESTING
        char* instructionString = LLVMPrintValueToString(instruction);
        //printf("candidate: %s\n", instructionString);

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
                    newOperands.push_back(newOperand);
                }

                // Extracting operands of old instruction
                vector<LLVMValueRef> oldOperands;
                int oldNumOperands = LLVMGetNumOperands(oldInst);

                for (int i = 0; i < oldNumOperands; i++) {
                    LLVMValueRef oldOperand = LLVMGetOperand(oldInst, i);
                    char* op = LLVMPrintValueToString(oldOperand);
                    oldOperands.push_back(oldOperand);
                }

                // If operands of new and old instructions match, 
                if (newOperands == oldOperands) {
                    char* testTwo = LLVMPrintValueToString(oldInst);
                    //printf("old: %s\n", testTwo);

                    // If instruction is a load instruction, check for stores
                    if (opcode == LLVMLoad) {
                        for (auto iterTwo = iter; iterTwo != oldInstructions.end(); ++iterTwo) {

                            LLVMOpcode instOpcode = LLVMGetInstructionOpcode(*iterTwo);
                            if (instOpcode == LLVMStore) {

                                // Retrieve memory location that store instruction writes to 
                                LLVMValueRef oldStoreLoc = LLVMGetOperand(*iterTwo, 1);

                                // TESTING
                                char* oldStoreLocStr = LLVMPrintValueToString(oldStoreLoc);
                                char* newStoreLocStr = LLVMPrintValueToString(newOperands.at(0));
                                //printf("memloc1: %s\n", oldStoreLocStr);
                                //printf("memloc2: %s\n", newStoreLocStr);

                                // If store instruction does not write to same memory location as load, replace load
                                if (oldStoreLoc != newOperands.at(0)) {

                                    // TESTING
                                    //printf("Common subexpression DETECTED\n\n");


                                    LLVMReplaceAllUsesWith(instruction, oldInst);
                                }
                                else {
                                    // TESTING
                                    //printf("Common subexpression NOT DETECTED\n\n");
                                }
                            }
                        }
                    }

                    // Otherwise is a common subexpression
                    else {
                        if (opcode != LLVMAlloca) {
                            LLVMReplaceAllUsesWith(instruction, oldInst);

                            // TESTING
                            //printf("Common subexpression DETECTED\n\n");
                        }
                    }
                    breakHelper = 1;
                    break;
                }
            }
        }
        if (breakHelper == 0) {
            char* test = LLVMPrintValueToString(instruction);

            // TESTING
            //printf("Common subexpression NOT DETECTED\n\n");

            oldInstructions.push_back(instruction);
        }
        breakHelper = 0;
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
                    LLVMReplaceAllUsesWith(instruction, constant);
                    break;
                case LLVMSub:
                    constant = LLVMConstSub(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    LLVMReplaceAllUsesWith(instruction, constant);
                    break;
                case LLVMMul:
                    constant = LLVMConstMul(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    LLVMReplaceAllUsesWith(instruction, constant);
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
            if (!genSet.empty()) {
                auto oldInst = genSet.begin();

                while (oldInst != genSet.end()) {
                    LLVMOpcode instRefOpCode = LLVMGetInstructionOpcode(*oldInst);

                    if (instRefOpCode == LLVMStore) {
                        LLVMValueRef memLoc = LLVMGetOperand(*oldInst, 1);

                        // Remove instruction that is killed
                        if (memLoc == LLVMGetOperand(instruction, 1)) {

                            // Does this erase a copy or the original???
                            genSet.erase(*oldInst);
                        }
                        ++oldInst;
                    }
                }
            }
            // Insert instruction into gen set
            genSet.insert(instruction);
        }
        // what if three store to same location

    }
    return genSet;
}











// kill set for all stores or just those in successors

/***************** computeKill ***********************/
set<LLVMValueRef> computeKill(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, vector<LLVMValueRef>> allStoreSet) 
{
    // Initializing kill set to empty set
    set<LLVMValueRef> killSet; 

    // Iterate over all instructions in basic block
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
            instruction != NULL; 
            instruction = LLVMGetNextInstruction(instruction)) {
        
        if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {
            LLVMValueRef memLoc = LLVMGetOperand(instruction, 1);
            int check = allStoreSet.count(memLoc);

            if (check > 0) {
                vector<LLVMValueRef> locations = (allStoreSet.find(memLoc))->second;

                for (auto it = locations.begin(); it != locations.end(); ++it) {
                    
                    // Make sure that instruction does not kill itself
                    if (*it != instruction) {
                        killSet.insert(*it);
                    }
                }
            }
        }
    }
    return killSet;
}




/***************** computeInandOut ***********************/
vector<set<LLVMValueRef>> computeInandOut(LLVMValueRef function, basicBlockSets_t completeSets) 
{
    // Determine predecessors of each basic block
    unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predecessorMap = computePredecessors(function);

    // Create maps to store "in" sets and "out" sets for each basic block
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> inSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> outSets;

    // Set "out" set of each block equal to its "gen" set
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL;
            bb = LLVMGetNextBasicBlock(bb)) {

        auto originalGenSet = completeSets.genSets.find(bb);

        if (originalGenSet != completeSets.genSets.end()) {
            set<LLVMValueRef> extractedGenSet = originalGenSet->second;
            outSets.insert({bb, extractedGenSet});
        }
    }


    // // TESTING
    // for (LLVMBasicBlockRef bb2 = LLVMGetFirstBasicBlock(function); bb2 != NULL;
    //         bb2 = LLVMGetNextBasicBlock(bb2)) {

    //         auto itTest = outSets.find(bb2);

    //         if (itTest != outSets.end()) {
    //             //TESTING
    //             printf("outset test:");
    //             fflush(stdout);
    //             printSet(itTest->second);
    //             printf("\n");
    //         }
    // }
    // printf("test");
    // fflush(stdout);

    bool change = true;
    while(change) {

        // Calculating "in" set of each basic block from union of out sets
        int firstInstCheck = 0;

        // Iterating through each basic block
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL;
                bb = LLVMGetNextBasicBlock(bb)) {

            printf("Loop0");

            // Skip first basic block, which has no predecessors
            if (firstInstCheck == 0) {
                firstInstCheck++;
                continue;
            }

            // Retrieving predecessors of basic block
            auto iter = predecessorMap.find(bb);

            if (iter != predecessorMap.end()) {
                printf("Loop1");
                vector<LLVMBasicBlockRef> predecessors = iter->second;
                set<LLVMValueRef> setOne;
                fflush(stdout);

                // Find union of the predecessor out sets
                for (auto iterTwo = predecessors.begin(); iterTwo != predecessors.end(); ++iterTwo) {

                    // Retrieving a predecessor's out set
                    auto iterThree = outSets.find(*iterTwo);

                    if (iterThree != outSets.end()) {
                        printf("Loop2\n");
                        set<LLVMValueRef> setTwo = iterThree->second;

                        printf("Set one: \n");



                        printSet(setOne);
                        printf("Set two: \n");
                        printSet(setTwo);




                        setOne = findSetUnion(setOne, setTwo);
                    }
                }

                printf("Final union: \n");
                printSet(setOne);
                fflush(stdout);

                // Setting "in" set of basic block equal to the union of predecessor sets

                // should create insets not the one from complete
                completeSets.inSets.insert({bb, setOne});
                auto oldoutIter = outSets.find(bb);

                if (oldoutIter != outSets.end()) {
                    printf("test5");
                    fflush(stdout);
                    set<LLVMValueRef> oldout = oldoutIter->second;

                    auto genSetIter = completeSets.genSets.find(bb);

                    if (genSetIter != completeSets.genSets.end()) {
                        printf("test6");
                        fflush(stdout);
                        set<LLVMValueRef> genSet = genSetIter->second;

                        // WORKS UNTIL HERE

                        // Calculating new "out" set for basic block
                        auto inSetIter = completeSets.inSets.find(bb);

                        if (inSetIter != completeSets.inSets.end()) {
                            printf("test7");
                            fflush(stdout);
                            set<LLVMValueRef> inSet = inSetIter->second;

                            auto killSetIter = completeSets.killSets.find(bb);

                            if (killSetIter != completeSets.killSets.end()) {
                                printf("test8");
                                fflush(stdout);
                                set<LLVMValueRef> killSet = killSetIter->second;

                                printf("\ninset before:\n");
                                printSet(inSet);
                                inSet = setMinus(inSet, killSet);

                                printf("\nkillset: \n");
                                printSet(killSet);

                                printf("\ninset after:\n");
                                printSet(inSet);

                                auto newOutIter = outSets.find(bb);

                                if (newOutIter != outSets.end()) {
                                    printf("test9");
                                    fflush(stdout);
                                    outSets.find(bb)->second = findSetUnion(genSet, inSet);
                                }


                                // START HERE

                                auto changeSet = outSets.find(bb);

                                if (changeSet != outSets.end()) {
                                    printf("test10");
                                    fflush(stdout);
                                    if (outSets.find(bb)->second == oldout) { 
                                        change = false; 
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //printVectorTwo(predecessorMap);
}





/***************** computePredecessors ***********************/
unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> computePredecessors(LLVMValueRef function) 
{
    // Initializing map to store the predecessors of each basic block
    unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predecessorMap;

    // Iterating through basic blocks
    LLVMBasicBlockRef basicBlock;
    
    for (basicBlock = LLVMGetFirstBasicBlock(function); 
            basicBlock != NULL; 
            basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        // Retrieving successor blocks of basic block
        LLVMValueRef terminator = LLVMGetLastInstruction(basicBlock);

        if (LLVMGetInstructionOpcode(terminator) == LLVMBr) {
            int numOperands = LLVMGetNumOperands(terminator);

            for (int i = 0; i < numOperands; i++) {

                // SHORTEN WITH get successor

                // Extracting reference to successor basic block from the operand
                LLVMValueRef successorLoc = LLVMGetOperand(terminator, i);
                
                if (LLVMIsABasicBlock(successorLoc) != NULL) {
                    LLVMBasicBlockRef successor = LLVMValueAsBasicBlock(successorLoc);

                    // Adding basic block as a predecessor using successor reference as a key
                    auto item = predecessorMap.find(successor);
                    if (item != predecessorMap.end()) {
                        (item->second).push_back(basicBlock);
                    }
                    else {
                        vector<LLVMBasicBlockRef> predecessors;
                        predecessors.push_back(basicBlock);
                        predecessorMap.insert({successor, predecessors});
                    }
                }
            }
        }
    }
    return predecessorMap;
}



/***************** findSetUnion ***********************/
set<LLVMValueRef> findSetUnion(set<LLVMValueRef> setOne, set<LLVMValueRef> setTwo) 
{
    // Returning empty set if one of the sets is empty
    if (setOne.empty()) { return setTwo; }
    if (setTwo.empty()) { return setOne; }

    // Determining relative sizes of sets
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

    set<LLVMValueRef> union_set;

    // Iterate through smaller set
    for (auto it = smaller.begin(); it != smaller.end(); ++it) {

        LLVMValueRef instruction = *it;

        if (larger.find(instruction) != larger.end()) {
            union_set.insert(instruction);
        }
    }
    return union_set;
}


/***************** setMinus ***********************/
set<LLVMValueRef> setMinus(set<LLVMValueRef> inSet, set<LLVMValueRef> killSet) 
{
    for (auto it = killSet.begin(); it != killSet.end(); ++it) {
        if (inSet.find(*it) != inSet.end()) {
            inSet.erase(*it);
        }
    }
    return inSet;
}











/***************** constantPropagation ***********************/
bool constantPropagation(LLVMValueRef function)
{
    // Initializing struct to hold "gen", "kill", "in", "out" sets
    basicBlockSets_t completeBBSet;

    // Computing set "S" containing all store instructions in function
    unordered_map<LLVMValueRef, vector<LLVMValueRef>> allStoreSet;

    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			basicBlock;
  			basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); 
                instruction != NULL; 
                instruction = LLVMGetNextInstruction(instruction)) {

            if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {

                // Checking if store instruction with same operand has already been visited
                LLVMValueRef memLoc = LLVMGetOperand(instruction, 1);
                auto item = allStoreSet.find(memLoc);

                // If same operand already visited, add instruction to vector
                if (item != allStoreSet.end()) {
                    (item->second).push_back(instruction);
                }
                // Otherwise add operand as a key along with instruction
                else {
                    vector<LLVMValueRef> storeSet;
                    storeSet.push_back(instruction);
                    allStoreSet.insert({memLoc, storeSet});
                }
            }
        }

        // Creating "gen" set and "kill" set for each basic block
        completeBBSet.genSets.insert({basicBlock, computeGen(basicBlock)});
        completeBBSet.killSets.insert({basicBlock, computeKill(basicBlock, allStoreSet)});
    }





        //TESTING - PRINT OUT gen and kill
        // for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 		// 	basicBlock;
  		// 	basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        //         auto it = completeBBSet.genSets.find(basicBlock);

        //         printf("Gen set:\n");
        //         if (it != completeBBSet.genSets.end()) {
        //             printSet(it->second);
        //         }
        //         else {
        //             printf("empty\n");
        //         }

                // auto it2 = completeBBSet.killSets.find(basicBlock);

                // printf("Kill set:\n");
                // if (it2 != completeBBSet.killSets.end()) {
                //     printSet(it2->second);
                // }
                // else {
                //     printf("empty\n");
                // }
    //}
    // NOT TESTING
    computeInandOut(function, completeBBSet);

    //TESTING
    // printf("All store\n");
    // printVector(allStoreSet);
    // printf("\n\n");
    // for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 	// 		basicBlock;
  	// 		basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
    //     printf("\n");
        
    //     printf("GEN:\n");
    //     printSet(completeBBSet.genSets.at(basicBlock));
    //     printf("KILL:\n");
    //     printSet(completeBBSet.killSets.at(basicBlock));
    // }
    // printf("\n\n");

}

















// Testing
void printSet(set<LLVMValueRef> testSet) {

    for (auto it = testSet.begin(); it != testSet.end(); ++it) {

        char* test = LLVMPrintValueToString(*it);
        printf("%s\n", test);

    }
}

void printVector(unordered_map<LLVMValueRef, vector<LLVMValueRef>> allStoreSet) {

    for (auto it = allStoreSet.begin(); it != allStoreSet.end(); ++it) {
        
        LLVMValueRef test = it->first;
        char* testtest = LLVMPrintValueToString(test);
        printf("%s\n", testtest);
        vector<LLVMValueRef> testTwo = it->second;

        for (auto iter = testTwo.begin(); iter != testTwo.end(); ++iter) {
            char* testThree = LLVMPrintValueToString(*iter);
            printf("%s\n", testThree);
        }
        printf("\n");
    }

}

// WHAT do we do for return terminators
void printVectorTwo(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> allStoreSet) {

    for (auto it = allStoreSet.begin(); it != allStoreSet.end(); ++it) {
        
        LLVMBasicBlockRef test = it->first;
        char* testtest = LLVMPrintValueToString(LLVMBasicBlockAsValue(test));
        printf("PARENT: \n");
        printf("%s\n", testtest);
        vector<LLVMBasicBlockRef> testTwo = it->second;

        printf("PREDECESSORS: \n");
        for (auto iter = testTwo.begin(); iter != testTwo.end(); ++iter) {
            char* testThree = LLVMPrintValueToString(LLVMBasicBlockAsValue(*iter));
            printf("%s\n", testThree);
        }
        //printf("\n");
        fflush(stdout);
    }

}
