/**
 * optimizations.c
 * 
 * 
 * 
*/


// constant prop: get rid of loads

/***************** dependencies ***********************/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include "optimizations.h"
using namespace std;


/***************** global function declarations ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb);
bool constantFolding(LLVMValueRef function);
bool constantPropagation(LLVMValueRef function);
bool cleanDeadCode(LLVMValueRef function);


/***************** local function declarations ***********************/
static set<LLVMValueRef> setMinus(set<LLVMValueRef> inSet, set<LLVMValueRef> killSet);
static set<LLVMValueRef> findSetUnion(set<LLVMValueRef> setOne, set<LLVMValueRef> setTwo); 
static unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> computePredecessors(LLVMValueRef function);
static set<LLVMValueRef> computeGen(LLVMBasicBlockRef bb);
static set<LLVMValueRef> computeKill(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, vector<LLVMValueRef>> allStoreSet);
static vector<set<LLVMValueRef>> computeInandOut(LLVMValueRef function, vector<LLVMBasicBlockRef> predecessors, 
        basicBlockSets_t completeBBSets, LLVMBasicBlockRef basicBlock);
static void deleteLoadInsts(basicBlockSets_t completeBBSet, LLVMValueRef function);


/***************** test function declarations ***********************/
static void printSet(set<LLVMValueRef> testSet);
static void printAllSets(basicBlockSets_t completeBBSet);
static char extractBlockName(char* bbAsString);




void printVector(unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>> allStoreSet);
void printVectorTwo(unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> allStoreSet);















/* Section 1: Local Optimizations */

/***************** removeCommonSubexpression ***********************/
bool removeCommonSubexpression(LLVMBasicBlockRef bb) 
{
    // Initializing vector to store instructions already encountered
    vector<LLVMValueRef> oldInstructions;
    int breakHelper = 0;

    // Iterating through basic block instructions
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
            instruction != NULL; 
            instruction = LLVMGetNextInstruction(instruction)) {

        // TESTING
        // char* instructionString = LLVMPrintValueToString(instruction);
        // printf("candidate: %s\n", instructionString);

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

                    // TESTING
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
                                // printf("memloc1: %s\n", oldStoreLocStr);
                                // printf("memloc2: %s\n", newStoreLocStr);

                                // If store instruction does not write to same memory location as load, replace load
                                if (oldStoreLoc != newOperands.at(0)) {
                                    // TESTING
                                    //printf("Common subexpression DETECTED\n\n");
                                    LLVMReplaceAllUsesWith(instruction, oldInst);
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
    return true;
}


/***************** cleanDeadCode ***********************/
bool cleanDeadCode(LLVMValueRef function)
{
    // Initializing vector to hold instructions-to-be-deleted
    vector<LLVMValueRef> instToDel;

    // Iterating through each basic block
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); 
             basicBlock;
             basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        // Iterating through instruction in the basic block
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); 
                instruction != NULL; 
                instruction = LLVMGetNextInstruction(instruction)) {

            // Store, ret, br, and call instructions should not be removed
            bool validInst = true;

            LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
            switch (opcode) {
                case LLVMStore:
                    validInst = false; 
                    break;
                case LLVMRet:
                    validInst = false; 
                    break;
                case LLVMBr:
                    validInst = false; 
                    break;
                case LLVMCall:
                    validInst = false; 
                    break;
                case LLVMAlloca:
                    validInst = false;
                    break;
                default:
                    validInst = true;
                    break;
            }

            // Collecting unused instructions in vector
            if (validInst) {
                LLVMUseRef instUse = LLVMGetFirstUse(instruction);
                if (instUse == NULL) {
                    instToDel.push_back(instruction);
                }
            }
        }   
    }

    for (auto delIter = instToDel.begin(); delIter != instToDel.end(); ++delIter) {
        LLVMInstructionRemoveFromParent(*delIter);
    }
    return true;
}


/***************** constantFolding ***********************/
bool constantFolding(LLVMValueRef function)
{
    bool change = false;
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
            LLVMValueRef constant;
            bool store = false;
            LLVMValueRef instructionHold;

            // TESTING
            char* test = LLVMPrintValueToString(instruction);
            printf("inst: %s\n", test);
            int numOper = LLVMGetNumOperands(instruction);
            for (int i = 0; i < numOper; ++i) {
                char* asf = LLVMPrintValueToString(LLVMGetOperand(instruction, i));
                printf("    operand: %s\n", asf);

                if (LLVMIsABinaryOperator(LLVMGetOperand(instruction, i)) != NULL) {
                    printf("ADD\n");
                }
            }



            if (opCode == LLVMStore) {
                LLVMValueRef embeddedInst = LLVMGetOperand(instruction, 0);
                opCode = LLVMGetInstructionOpcode(embeddedInst);
                if (LLVMIsAInstruction(embeddedInst) != NULL && opCode == LLVMAdd) {
                    printf("ASDFASDF\n");
                    char* test = LLVMPrintValueToString(embeddedInst);
                    printf("embed: %s\n", test);
                    instructionHold = instruction;
                    instruction = embeddedInst;
                    store = true;
                }
            }



            // If instruction is add, subtract or multiply, replace with constant instruction
            switch (opCode) {
                case LLVMAdd:
                    constant = LLVMConstAdd(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    LLVMReplaceAllUsesWith(instruction, constant);
                    change = true;
                    break;
                case LLVMSub:
                    constant = LLVMConstSub(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    LLVMReplaceAllUsesWith(instruction, constant);
                    change = true;
                    break;
                case LLVMMul:
                    constant = LLVMConstMul(LLVMGetOperand(instruction, 0), LLVMGetOperand(instruction, 1));
                    LLVMReplaceAllUsesWith(instruction, constant);
                    change = true;
                    break;
            }

            if (store) {
                instruction = instructionHold;

            }        
        }
    }
    // Returning true if optimizations made and false if no optimizations added
    return !change;
}



/* Section 2: Global Optimizations */

/***************** computeGen ***********************/
static set<LLVMValueRef> computeGen(LLVMBasicBlockRef bb) 
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
    }
    return genSet;
}


/***************** computeKill ***********************/
static set<LLVMValueRef> computeKill(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, vector<LLVMValueRef>> allStoreMap) 
{
    // Initializing kill set to empty set
    set<LLVMValueRef> killSet; 

    // Iterate over all instructions in basic block
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
            instruction != NULL; 
            instruction = LLVMGetNextInstruction(instruction)) {
        
        if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {
            LLVMValueRef memLoc = LLVMGetOperand(instruction, 1);
            int check = allStoreMap.count(memLoc);

            if (check > 0) {
                vector<LLVMValueRef> locations = (allStoreMap.find(memLoc))->second;

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
static vector<set<LLVMValueRef>> computeInandOut(LLVMValueRef function, vector<LLVMBasicBlockRef> predecessors, 
        basicBlockSets_t completeBBSets, LLVMBasicBlockRef basicBlock) 
{
    // Initializing block "out" and "in" sets
    set<LLVMValueRef> blockOutSet;
    set<LLVMValueRef> blockInSet;
    set<LLVMValueRef> blockGenSet;

    // Initializing vector that will eventually hold "out" and "in" sets
    vector<set<LLVMValueRef>> newInAndOut;

    // Retrieving block's "out" and "gen" sets
    auto blockGenSetIter = completeBBSets.genSets.find(basicBlock);
    if (blockGenSetIter != completeBBSets.genSets.end()) {
        blockGenSet = blockGenSetIter->second;

        auto blockOutSetIter = completeBBSets.outSets.find(basicBlock);
        if (blockOutSetIter != completeBBSets.outSets.end()) {
            blockOutSet = blockOutSetIter->second;
        
            // Calculating "in" set of each basic block from union of "out" set
            set<LLVMValueRef> setOne;

            // Iterating through predecessor vector and computing the union of each predecessor's "out" set
            for (auto predVecIter = predecessors.begin(); predVecIter != predecessors.end(); ++predVecIter) {
                auto predOutIter = completeBBSets.outSets.find(*predVecIter);

                if (predOutIter != completeBBSets.outSets.end()) {
                    set<LLVMValueRef> setTwo = predOutIter->second;
                    setOne = findSetUnion(setOne, setTwo);
                }
            }

            // Setting "in" set of basic block equal to the union of predecessor sets
            blockInSet = setOne;

            // Saving old "out" set of block
            set<LLVMValueRef> oldOut = blockOutSet;

            
            set<LLVMValueRef> genSet = blockGenSet;

            // Retrieving the block's "kill" set
            auto killSetIter = completeBBSets.killSets.find(basicBlock);
            if (killSetIter != completeBBSets.killSets.end()) {
                set<LLVMValueRef> killSet = killSetIter->second;

                /* Calculating new "out" set for basic block using equation 
                OUT[B] = GEN[B] U (IN[B] - KILL[B]) */
                set<LLVMValueRef> inKillDiff = setMinus(blockInSet, killSet);
                set<LLVMValueRef> genUnionDiff = findSetUnion(genSet, inKillDiff);

                // Return "in" set, "out" set, and "oldout" set
                set<LLVMValueRef> newOut = genUnionDiff;
                newInAndOut.push_back(newOut);
                newInAndOut.push_back(blockInSet);
                newInAndOut.push_back(oldOut);

                return newInAndOut;
            }
        }
    }
    return vector<set<LLVMValueRef>>();
}



/***************** computePredecessors ***********************/
static unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> computePredecessors(LLVMValueRef function) 
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

    // Since first basic block has no predecessor, it is added to the map with an empty set
    predecessorMap.insert({LLVMGetFirstBasicBlock(function), vector <LLVMBasicBlockRef> ()});
    return predecessorMap;
}



/***************** findSetUnion ***********************/
static set<LLVMValueRef> findSetUnion(set<LLVMValueRef> setOne, set<LLVMValueRef> setTwo) 
{
    // Returning non-empty set if one of the sets is empty
    if (setOne.empty()) { return setTwo; }
    if (setTwo.empty()) { return setOne; }

    set<LLVMValueRef> union_set = setOne;

    // Iterate through smaller set
    for (auto setIter = setTwo.begin(); setIter != setTwo.end(); ++setIter) {
        LLVMValueRef instruction = *setIter;
        union_set.insert(instruction);
    }
    return union_set;
}


/***************** setMinus ***********************/
static set<LLVMValueRef> setMinus(set<LLVMValueRef> inSet, set<LLVMValueRef> killSet) 
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
    unordered_map<LLVMValueRef, vector<LLVMValueRef>> allStoreMap;

    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			basicBlock;
  			basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); 
                instruction != NULL; 
                instruction = LLVMGetNextInstruction(instruction)) {

            if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {

                // Checking if store instruction with same operand has already been visited
                LLVMValueRef memLoc = LLVMGetOperand(instruction, 1);
                auto item = allStoreMap.find(memLoc);

                // If same operand already visited, add instruction to vector
                if (item != allStoreMap.end()) {
                    vector<LLVMValueRef> updatedVector = item->second;
                    updatedVector.push_back(instruction);
                    allStoreMap.erase(memLoc);
                    allStoreMap.insert({memLoc, updatedVector});
                }
                // Otherwise add operand as a key along with instruction
                else {
                    vector<LLVMValueRef> storeSet;
                    storeSet.push_back(instruction);
                    allStoreMap.insert({memLoc, storeSet});
                }
            }
        }
    }

    // Creating "gen" set and "kill" set for each basic block
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			basicBlock;
  			basicBlock = LLVMGetNextBasicBlock(basicBlock)) {

        completeBBSet.genSets.insert({basicBlock, computeGen(basicBlock)});
        completeBBSet.killSets.insert({basicBlock, computeKill(basicBlock, allStoreMap)});
    }

    // Determine predecessors of each basic block
    unordered_map<LLVMBasicBlockRef, vector<LLVMBasicBlockRef>> predecessorMap = computePredecessors(function);

    // Clearing "in" and "out" sets from previous iterations of constant propagation
    completeBBSet.inSets.clear();
    completeBBSet.outSets.clear();

    // Setting each block's "out" set equal to its "gen" set
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL;
            bb = LLVMGetNextBasicBlock(bb)) {

        // Retrieving block's "gen" set
        auto blockGenSetIter = completeBBSet.genSets.find(bb);
        if (blockGenSetIter != completeBBSet.genSets.end()) {
            set<LLVMValueRef> genSet = blockGenSetIter->second;

            // Setting block's "out" set equal to its "gen" set
            auto blockOutSetIter = completeBBSet.outSets.find(bb);
            fflush(stdout);
            completeBBSet.outSets.insert({bb, genSet});
        }
    }

    LLVMValueRef asdf = function;
    char* func_old = LLVMPrintValueToString(asdf);
    //printf("%s\n", func_old);


    bool change = true;
    while (change) {
        change = false;

        // Initializing map to hold sets above keyed by basic block reference
        unordered_map<LLVMBasicBlockRef, vector<set<LLVMValueRef>>> updatedSets;

        // Iterating through each basic block
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL;
                bb = LLVMGetNextBasicBlock(bb)) {
            
            // Retrieving vector of predecessors for basic block
            auto predIter = predecessorMap.find(bb);

            // Compute "in" and "out" sets for basic block
            if (predIter != predecessorMap.end()) {
                vector<LLVMBasicBlockRef> predecessors = predIter->second;
                vector<set<LLVMValueRef>> inAndOutSets = computeInandOut(function, predecessors, completeBBSet, bb);

                // Add updated sets to map, which will hold them until all blocks have been iterated through
                updatedSets.insert({bb, inAndOutSets});

                // Initializing sets to hold "out" set, "in" set, and old "out" set
                set<LLVMValueRef> outSet = inAndOutSets.at(0);
                set<LLVMValueRef> oldOutSet = inAndOutSets.at(2);

                if (oldOutSet != outSet) {
                    change = true;
                }    
            }
        }

        // Update completeBBSets
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL;
                bb = LLVMGetNextBasicBlock(bb)) {
            
            // Inserting updated "out" set into complete sets struct

            // Retrieving updated sets
            auto updateSetIter = updatedSets.find(bb);
            if (updateSetIter != updatedSets.end()) {
                vector<set<LLVMValueRef>> blockUpdateSet = updateSetIter->second;

                // Initializing sets to hold "out" set, "in" set, and old "out" set
                set<LLVMValueRef> outSet = blockUpdateSet.at(0);
                set<LLVMValueRef> inSet = blockUpdateSet.at(1);
                set<LLVMValueRef> oldOutSet = blockUpdateSet.at(2);

                // Inserting updated "out" set into complete sets struct
                auto outSetIter = completeBBSet.outSets.find(bb);
                if (outSetIter != completeBBSet.outSets.end()) {
                    outSetIter->second = outSet;
                }
                else {
                    completeBBSet.outSets.insert({bb, outSet});
                }

                // Inserting updated "in" set into complete sets struct
                auto inSetIter = completeBBSet.inSets.find(bb);
                if (inSetIter != completeBBSet.inSets.end()) {
                    inSetIter->second = inSet;
                }
                else {
                    completeBBSet.inSets.insert({bb, inSet});
                }
                deleteLoadInsts(completeBBSet, function);
            }
        }
    }
    return true;
}

//LLVMGetFirstUse



/***************** deleteLoadInsts ***********************/
static void deleteLoadInsts(basicBlockSets_t completeBBSet, LLVMValueRef function)
{
    // Initializing vector to hold set R for each basic block
    vector<set<LLVMValueRef>> rSets;
    unordered_map<LLVMBasicBlockRef, LLVMValueRef> markedDelete;

    // Iterating through each basic block
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL;
            bb = LLVMGetNextBasicBlock(bb)) {
        
        // Retrieving "in" set for the basic block
        set<LLVMValueRef> rSet;
        auto rSetIter = completeBBSet.inSets.find(bb);

        if (rSetIter != completeBBSet.inSets.end()) {
            rSet = rSetIter->second;
        }
        else {
            rSet = set<LLVMValueRef>();
        }

        // Iterating through each instruction in the basic block
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); 
                instruction != NULL; 
                instruction = LLVMGetNextInstruction(instruction)) {
            
            // If instruction is a store instruction
            if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {

                // Adding it to rSet
                rSet.insert(instruction);

                // Removing store instructions in rSet that are killed by instruction
                LLVMValueRef memLoc = LLVMGetOperand(instruction, 1);
                set<LLVMValueRef> rSetCopy = rSet;

                for (auto checkIter = rSet.begin(); checkIter != rSet.find(instruction); ++checkIter) {
                    LLVMValueRef storeInst = *checkIter;
                    LLVMValueRef memLocCheck = LLVMGetOperand(storeInst, 1);

                    if (memLoc == memLocCheck) {
                        rSetCopy.erase(storeInst);
                    }
                }
                rSet = rSetCopy;
            
            
            printf("Rset: \n");

            //printSet(rSet);

            }
        

        

                // If instruction is a load instruction
                else if (LLVMGetInstructionOpcode(instruction) == LLVMLoad) {

                    // Finding store instructions in R that write to same memory location as instruction
                    LLVMValueRef memLoc = LLVMGetOperand(instruction, 0);
                    char* testStoreOne = LLVMPrintValueToString(instruction);
                    printf("INSTRUCTION: %s\n", testStoreOne);
                    fflush(stdout);
                    //if (memLoc != NULL) {
                        char* testStoreTwo = LLVMPrintValueToString(memLoc);
                        printf("INSTRUCTION: %s\n", testStoreTwo);
                        fflush(stdout);

                    //}
                    fflush(stdout);
                    vector<LLVMValueRef> sameLocStore;

                    //printf("INASDDGT: %s\n", testStoreTwo);

                    // Finding all the store instructions in R that write to same address
                    for (auto sameLocStoreIter = rSet.begin(); sameLocStoreIter != rSet.end(); ++sameLocStoreIter) {
                        LLVMValueRef storeInst = *sameLocStoreIter;


                        // char* testStore = LLVMPrintValueToString(storeInst);
                        // printf("STORE: %s\n", testStore);
                        // char* testStoreTwo = LLVMPrintValueToString(LLVMGetOperand(storeInst, 1));
                        // printf("MEMLOC: %s\n", testStoreTwo);


                        if (LLVMGetOperand(storeInst, 1) == memLoc) {
                            printf("THISISTEST\n");
                            sameLocStore.push_back(storeInst);
                        }
                    }

                    // If all these store instructions store the same constant
                    bool same = true;
                    int i = 0;
                    auto sameIter = sameLocStore.begin();
                    LLVMValueRef holder;
                    LLVMValueRef constOp;


                    for (auto sameIter = sameLocStore.begin(); sameIter != sameLocStore.end(); ++sameIter) {

                        printf("sameIter = %s\n", LLVMPrintValueToString(*sameIter));
                        fflush(stdout);
                        constOp = LLVMGetOperand(*sameIter, 0);

                        // //ISSUE
                        if (LLVMIsAConstant(constOp) != NULL) {
                        //         constOp = LLVMGetOperand(*sameIter, 0);

                            if (i == 0) {
                                printf("first\n");
                                holder = constOp;
                                i++;
                            }

                            

                                if (holder != LLVMGetOperand(*sameIter, 0)) {
                                    same == false;
                                }
                        }
                }

                if (same && i == 1) {
                    // Replacing all uses of instruction with constant instruction
                    printf("ASDFASDF\n");




                    long long constVal = LLVMConstIntGetSExtValue(constOp);
                    LLVMTypeRef intType = LLVMInt32Type();
                    LLVMValueRef constStore = LLVMConstInt(intType, constVal, true);

                    for (auto sameIter = sameLocStore.begin(); sameIter != sameLocStore.end(); ++sameIter) {
                        char* test = LLVMPrintValueToString(constStore);
                        printf("prop: %s\n", test);
                        LLVMReplaceAllUsesWith(instruction, constStore);
                    }

                    // Marking old instruction for deletion
                    markedDelete.insert({bb, instruction});
                }
            }
                }
    }
    
    // Deleting all instructions marked for deletion
    for (auto delVecIter = markedDelete.begin(); delVecIter != markedDelete.end(); ++delVecIter) {
        LLVMValueRef markDel = delVecIter->second;

            LLVMInstructionEraseFromParent(markDel);
        
    }
}




























/* Section 3: Testing. The following functions were used for development and debugging */

/***************** printAllSets ***********************/
static void printAllSets(basicBlockSets_t completeBBSet) 
{
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> genSets = completeBBSet.genSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> killSets = completeBBSet.killSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> inSets = completeBBSet.inSets;
    unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> outSets = completeBBSet.outSets;

    printf("-----------------------------------------------------------\n");
    printf("GEN SETS: \n");
    printf("-----------------------------------------------------------\n");

    for (auto it = genSets.begin(); it != genSets.end(); ++it) {
        LLVMBasicBlockRef blockName = it->first;
        set<LLVMValueRef> blockSet = it->second;
        extractBlockName(LLVMPrintValueToString(LLVMBasicBlockAsValue(blockName)));
        printSet(blockSet);
    }

    printf("-----------------------------------------------------------\n");
    printf("KILL SETS: \n");
    printf("-----------------------------------------------------------\n");

    for (auto it = killSets.begin(); it != killSets.end(); ++it) {
        LLVMBasicBlockRef blockName = it->first;
        set<LLVMValueRef> blockSet = it->second;
        extractBlockName(LLVMPrintValueToString(LLVMBasicBlockAsValue(blockName)));
        printSet(blockSet);
    }

    printf("-----------------------------------------------------------\n");
    printf("IN SETS: \n");
    printf("-----------------------------------------------------------\n");

    for (auto it = inSets.begin(); it != inSets.end(); ++it) {
        LLVMBasicBlockRef blockName = it->first;
        set<LLVMValueRef> blockSet = it->second;
        extractBlockName(LLVMPrintValueToString(LLVMBasicBlockAsValue(blockName)));
        printSet(blockSet);
    }

    printf("-----------------------------------------------------------\n");
    printf("OUT SETS: \n");
    printf("-----------------------------------------------------------\n");

    for (auto it = outSets.begin(); it != outSets.end(); ++it) {
        LLVMBasicBlockRef blockName = it->first;
        set<LLVMValueRef> blockSet = it->second;
        extractBlockName(LLVMPrintValueToString(LLVMBasicBlockAsValue(blockName)));
        printSet(blockSet);
    }
    fflush(stdout);
}


/***************** printSet ***********************/
static void printSet(set<LLVMValueRef> testSet) 
{
    for (auto it = testSet.begin(); it != testSet.end(); ++it) {
        char* test = LLVMPrintValueToString(*it);
        printf("%s\n", test);
    }
}


/***************** extractBlockName ***********************/
static char extractBlockName(char* bbAsString) {
    char blockName = bbAsString[1];
    char blockNameTwo = bbAsString[2];
    printf("\n\nBlock: %c%c\n\n", blockName, blockNameTwo);
    return blockName;
}








































void printVector(unordered_map<LLVMBasicBlockRef, vector<LLVMValueRef>> allStoreSet) {

    for (auto it = allStoreSet.begin(); it != allStoreSet.end(); ++it) {
        
        LLVMBasicBlockRef test = it->first;
        char* testtest = LLVMPrintValueToString(LLVMBasicBlockAsValue(test));  
        
        
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