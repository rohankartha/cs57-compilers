/**
 * assemblycodegen.c – Assembly code generator for the mini-c compiler
 * 
 * Rohan Kartha, April 2024
 * 
*/



/**************** Dependencies ****************/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include <cstddef>
#include <unordered_map>
#include <string>
#include "../front-end/ast/ast.h"
#include "../front-end/semantic-analysis.h"
#include <iostream>
#include <stack>
#include <set>
#include <unordered_map>
#include <string>
#include <algorithm>
using namespace std;



/**************** local functions ****************/
void computeLiveness(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, int>* instIndex, unordered_map<LLVMValueRef, array<int, 2>>* liveRange);
unordered_map<LLVMValueRef, string> allocateRegisters(LLVMValueRef function);
void generateAssemblyCode(FILE* fp, LLVMValueRef function, unordered_map<LLVMValueRef, string> registerAssignments);



/**************** local-global functions ****************/
LLVMValueRef findSpill(unordered_map<LLVMValueRef, string> registerAssignments, 
        unordered_map<LLVMValueRef, array<int, 2>> liveRange, vector<LLVMValueRef> sortedList,
        LLVMValueRef instruction);
vector<LLVMValueRef> sortInstructions(unordered_map<LLVMValueRef, array<int, 2>> liveRange);
unordered_map<LLVMBasicBlockRef, const char*> createBBLabels(LLVMValueRef function);
void printDirectives(FILE* fp, LLVMValueRef function);
void printFunctionEnd(FILE* fp);
void generateAssemblyArithmetic(FILE* fp, LLVMValueRef function, int localMem, 
        unordered_map<LLVMValueRef, string> registerAssignments, LLVMValueRef instruction,
        unordered_map<LLVMValueRef, int> offsetMap);
void generateAssemblyCompare(FILE* fp, LLVMValueRef function, int localMem, 
        unordered_map<LLVMValueRef, string> registerAssignments, LLVMValueRef instruction,
        unordered_map<LLVMValueRef, int> offsetMap);



/***************** computeLiveness ***********************/
void computeLiveness(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, int>* instIndex, unordered_map<LLVMValueRef, array<int, 2>>* liveRange)
{
    int index = -1;

    // Iterating through instructions in basic block
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst)) {
        
        // Incrementing instruction index counter
        index++;

        // Creating map containing instruction references and index of first appearances
        if (LLVMGetInstructionOpcode(inst) != LLVMAlloca &&
                LLVMGetInstructionOpcode(inst) != LLVMStore &&
                LLVMGetInstructionOpcode(inst) != LLVMBr &&
                LLVMGetInstructionOpcode(inst) != LLVMRet &&
                LLVMGetInstructionOpcode(inst) != LLVMCall) {

            instIndex->insert({inst, index});

            // Creating map encoding the range of each instruction
            array<int, 2> range;
            range[0] = index;

            // Calculating upper bound of liveness range for temporary variable
            bool use = false;
            int endIndex = index;
            int indexCopy = index;

            LLVMValueRef nextInst = LLVMGetNextInstruction(inst);

            // Iterating through rest of basic block to check liveness
            for (LLVMValueRef checkInst = nextInst; 
                    checkInst != NULL; checkInst = LLVMGetNextInstruction(checkInst)) {

                indexCopy++;
                int numOperands = LLVMGetNumOperands(nextInst);

                for (int i = 0; i < numOperands; i++) {
                    LLVMValueRef operand = LLVMGetOperand(checkInst, i);

                    // If temp variable is used, mark boolean as true and save index value
                    if (inst == operand) {
                        use = true;
                        endIndex = indexCopy;
                    }
                }
            }

            // Updating liveness map with range for temp variable
            range[1] = endIndex;
            liveRange->insert({inst, range});
        }
    }
}



/***************** allocateRegisters ***********************/
unordered_map<LLVMValueRef, string> allocateRegisters(LLVMValueRef function) 
{

    unordered_map<LLVMValueRef, string> registerAssignments;

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); 
            bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {

        // Initializing set of available registers
        set<string> availableRegisters;

        // Adding %ebx, %edx, and %ecx to set
        availableRegisters.insert(string("%ebx"));
        availableRegisters.insert(string("%edx"));
        availableRegisters.insert(string("%ecx"));

        // Calculating index map and range map
        unordered_map<LLVMValueRef, int>* instIndex = new unordered_map<LLVMValueRef, int>;
        unordered_map<LLVMValueRef, array<int, 2>>* liveRange = new unordered_map<LLVMValueRef, array<int, 2>>;
        computeLiveness(bb, instIndex, liveRange);

        int instructionNum = 0;

        // Iterating through instructions in basic block
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb);    
                instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
            
            LLVMOpcode instructionType = LLVMGetInstructionOpcode(instruction);
            instructionNum++;


            // If instruction is an allocate instruction, skip
            if (instructionType == LLVMAlloca) {
                continue;
            }

            
            // If instruction doesn't have a result

            // FIX CALL THAT DOESNT RETURN VALUE
            else if ((instructionType == LLVMAlloca ||
                    instructionType == LLVMStore ||
                    instructionType == LLVMBr ||
                    instructionType == LLVMRet ||
                    instructionType == LLVMCall)) {

                // Iterating through operands of instruction
                int numOperands = LLVMGetNumOperands(instruction);
                for (int i = 0; i < numOperands; i++) {

                    auto liveRangeIter = liveRange->find(instruction);
                    if (liveRangeIter != liveRange->end()) {

                        array<int, 2> operandLiveRange = liveRange->at(instruction); 
                        int endIndex = operandLiveRange[1];

                        // If any of the operands' liveness range ends at this instruction
                        if (endIndex == instructionNum) {

                            // If the instruction already assigned a register, add it to set of available registers
                            auto regIter = registerAssignments.find(instruction);

                            if (regIter != registerAssignments.end()) {
                                string assignedRegName = registerAssignments.at(instruction);
                                availableRegisters.insert(assignedRegName);
                            }
                            
                            // Adding register to availableRegisters set
                            LLVMValueRef operand = LLVMGetOperand(instruction, i);

                            string registerName = registerAssignments.at(operand);
                            availableRegisters.insert(registerName);

                        }
                    }
                }
            }

            else {

                /* Case 1: Instruction is of type add/sub/mul, its first operand 
                has register assigned, and first operand liveness range ends at instruction */

                bool inserted = false;

                if (instructionType == LLVMAdd || instructionType == LLVMSub || instructionType == LLVMMul) {

                    // NEED TO UPDATE AVAILABLE REGS MORE?

                    // Retrieving first operand
                    LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);

                    // Retrieving upper bound of temporary variable liveness range

                    auto findOpIter = liveRange->find(firstOperand);

                    if (findOpIter != liveRange->end()) {

                        int endIndex = liveRange->at(firstOperand)[1];

                        // Checking if first operand has assigned register
                        auto checkOper = registerAssignments.find(firstOperand);

                        /* If first operand has assigned register and its liveness range 
                        ends at parent instruction, assigning this register to parent instruction */
                        if (checkOper != registerAssignments.end() && endIndex == instructionNum) {


                            printf("add/sub/mul etc: %s\n\n", LLVMPrintValueToString(instruction));
                            fflush(stdout);

                            string registerName = registerAssignments.at(firstOperand);
                            registerAssignments.insert({instruction, registerName});


                            // Retrieving second operand
                            LLVMValueRef secondOperand = LLVMGetOperand(instruction, 1);

                            // Retrieving upper bound of temporary variable liveness range
                            int endIndexTwo = liveRange->at(secondOperand)[1];

                            // If liveness of operand ends at parent instruction, mark its register as available
                            if (endIndexTwo == instructionNum) {

                                auto secondOpIter = registerAssignments.find(secondOperand);
                                if (secondOpIter != registerAssignments.end()) {

                                    string regName = registerAssignments.at(secondOperand);
                                    availableRegisters.insert(regName);
                                    inserted = true;
                                }
                            }
                        }    
                    }
                }

                
                if (inserted) {
                    continue;
                }


                /* Case 2: Otherwise, if a physical register is available */

                if (!availableRegisters.empty()) {


                    printf("physical reg available: %s\n\n", LLVMPrintValueToString(instruction));
                    fflush(stdout);


                    // Retrieving available register name
                    string availableRegName = *availableRegisters.begin();

                    // Removing register name from available registers and adding it to assignment map
                    registerAssignments.insert({instruction, availableRegName});
                    availableRegisters.erase(availableRegName);


                    // Marking any registers used by operands at the end of their live ranges as available
                    for (int i = 0; i < LLVMGetNumOperands(instruction); i++) {

                        // Extracting operand and upper bound of its liveness range
                        LLVMValueRef operand = LLVMGetOperand(instruction, i);

                        auto findOperIndex = liveRange->find(operand);
                        if (findOperIndex != liveRange->end()) {

                            int operandEndIndex = (liveRange->at(operand))[1];

                            if (operandEndIndex == instructionNum) {

                                auto opIter = registerAssignments.find(operand);

                                if (opIter != registerAssignments.end()) {
                                    string newlyAvailableReg = registerAssignments.at(operand);
                                    availableRegisters.insert(newlyAvailableReg);
                                }
                            }
                        }
                    }
                }


                /* Case 3: Otherwise, if a physical register is not available */

                else {

                    printf("physical reg NOT available: %s\n\n", LLVMPrintValueToString(instruction));
                    fflush(stdout);

                    // Generating sorted list of instructions
                    vector<LLVMValueRef> sortedList = sortInstructions(*liveRange);

                    LLVMValueRef V = findSpill(registerAssignments, *liveRange, sortedList, instruction);



                    // TESTING
                    if (V != NULL) {
                        printf("spill: %s\n", LLVMPrintValueToString(V));
                        fflush(stdout);

                    }

                    int v_LivenessRange = liveRange->at(V).at(1);
                    int inst_LivenessRange = liveRange->at(instruction).at(1);


                    if (inst_LivenessRange > v_LivenessRange) {
                        registerAssignments.insert({instruction, "-1"});
                    }

                    else {
                        string registerName = registerAssignments.at(V);
                        registerAssignments.insert({instruction, registerName});

                        // Does this update???
                        registerAssignments.insert({V, "-1"});
                    }

                    // Marking any registers used by operands at the end of their live ranges as available
                    for (int i = 0; i < LLVMGetNumOperands(instruction); i++) {

                        LLVMValueRef operand = LLVMGetOperand(instruction, i);

                        if (liveRange->find(operand) != liveRange->end()) {

                            int operandEndIndex = liveRange->at(operand).at(1);

                            if (operandEndIndex == instructionNum) {
                                string newlyAvailableReg = registerAssignments.at(operand);
                                availableRegisters.insert(newlyAvailableReg);
                            }
                        }
                    }
                }
            }
        }
    }
    return registerAssignments;
} 



/***************** findSpill ***********************/
LLVMValueRef findSpill(unordered_map<LLVMValueRef, string> registerAssignments, 
        unordered_map<LLVMValueRef, array<int, 2>> liveRange, vector<LLVMValueRef> sortedList,
        LLVMValueRef instruction)
{

    // Calculating index of instruction in sorted list
    auto instIter = find(sortedList.begin(), sortedList.end(), instruction);
    int instIndex = distance(sortedList.begin(), instIter);

    // Iterating through every instruction in the sorted list
    for (int i = 0; sortedList.size(); i++) {
        LLVMValueRef instructionTwo = sortedList.at(i);

        int firstInstEnd;
        int secondInstBeg;

        // Determining relative order of two instructions
        if (i > instIndex) {
            firstInstEnd = liveRange.at(instruction).at(1);
            secondInstBeg = liveRange.at(instructionTwo).at(0);
        }
        else {
            firstInstEnd = liveRange.at(instructionTwo).at(1);
            secondInstBeg = liveRange.at(instruction).at(0);
        }

        // If the liveness ranges overlap
        if (secondInstBeg <= firstInstEnd) {

            // what if instructionTwo == instruction????????
            if (registerAssignments.find(instructionTwo) != registerAssignments.end()) {
                string registerName = registerAssignments.at(instructionTwo);

                if (registerName != "-1") {
                    return instructionTwo;
                }
            }
        }
    }
    return NULL;
}



/***************** sortInstructions ***********************/
vector<LLVMValueRef> sortInstructions(unordered_map<LLVMValueRef, array<int, 2>> liveRange)
{

    // Initializing vector to hold sorted instructions and string to instruction conversion map
    vector<string> unsortedInstructions;
    unordered_map<string, LLVMValueRef> stringToVal;

    // Initializing unsorted vector
    for (auto instIter = liveRange.begin(); instIter != NULL; ++instIter) {

        // Retrieving instruction
        LLVMValueRef instruction = instIter->first;

        // Adding instruction to vector and map
        string copy = LLVMPrintValueToString(instruction);
        stringToVal.insert({copy, instruction});
        unsortedInstructions.push_back(copy);
    }

    // Initializing vector to hold sorted instructions
    vector<string> sortedInstructions;

    // Iterating through all unsorted instructions
    for (auto vecIter = unsortedInstructions.begin(); vecIter != unsortedInstructions.end(); ++vecIter) {

        int i = 0;
        bool added = false;

        if (stringToVal.find(*vecIter) != stringToVal.end()) {

            // Extracting upper bound of liveness range
            LLVMValueRef unsortedValRef = stringToVal.at(*vecIter);
            int endIndex = liveRange.at(unsortedValRef)[1];

            // If sorted instructions vector is empty, adding the instruction
            if (sortedInstructions.empty()) {
                sortedInstructions.push_back(*vecIter);
                continue;
            }

            // Otherwise, iterating through sorted vector to find the correct index for insertion
            else {
                while (!added & i < sortedInstructions.size()) {

                    // Extracting LLVMValRef from sorted instruction in string format
                    string inst = sortedInstructions.at(i);
                    LLVMValueRef instLLVMVal = stringToVal.at(inst);

                    // Extracting upper bound of sorted instruction's liveness range
                    int endIndexCheck = liveRange.at(instLLVMVal)[1];

                    // Comparing unsorted and sorted instructions' upper bound of liveness range
                    if (endIndex > endIndexCheck ) {

                        added = true;
                        i--;
                    }
                    i++;
                }

                // Instruction added to internal index in vector
                if (added) {
                    sortedInstructions.insert(sortedInstructions.begin() + (i), *vecIter);
                }

                // Instruction added to end of vector
                else {
                    sortedInstructions.push_back(*vecIter);
                }
            }
        }
    }

    // Generating list of LLVMValueRefs
    vector<LLVMValueRef> sortedInstructionsVal;

    for (auto valIter = sortedInstructions.begin(); valIter != sortedInstructions.end(); ++valIter) {

        LLVMValueRef valFromString = stringToVal.at(*valIter);
        sortedInstructionsVal.push_back(valFromString);
    }

    // may need to reverse vector

    reverse(sortedInstructions.begin(), sortedInstructions.end());
    return sortedInstructionsVal;
}



/***************** getOffsetMap ***********************/
unordered_map<LLVMValueRef, int> getOffsetMap(LLVMValueRef function, int* localMem)
{
    // Initializing localMem (represents offset from %ebp)
    *localMem = 4;

    // Intializing offsetMap
    unordered_map<LLVMValueRef, int> offsetMap;
    LLVMValueRef funcParam = LLVMGetParam(function, 0);

    // Generating instruction to push parameter onto stack
    if (funcParam != NULL) {
        offsetMap.insert({funcParam, 8});
    }

    // Iterating through each basic block in the function
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL; 
            bb = LLVMGetNextBasicBlock(bb)) {
        
        // Iterating through each instruction in the basic block
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction != NULL;
                instruction = LLVMGetNextInstruction(instruction)) {

            LLVMOpcode instructionType = LLVMGetInstructionOpcode(instruction);

            switch (instructionType) {

                // If instruction is an allocate instruction
                case LLVMAlloca: {
                    *localMem = *localMem + 4;
                    offsetMap.insert({instruction, -1*(*localMem)});
                    break;
                }

                // If instruction is a store instruction
                case LLVMStore: {

                    // If instruction stores parameter value, set offset position for second operand as equal to same value
                    LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);
                    LLVMValueRef secondOperand = LLVMGetOperand(instruction, 1);

                    if (firstOperand == funcParam) {
                        int x = offsetMap.at(firstOperand);
                        auto secIter = offsetMap.find(secondOperand);
                        secIter->second = x;
                    }

                    // If instruction does not store parameter value
                    else {
                        int x = offsetMap.at(secondOperand);
                        offsetMap.insert({firstOperand, x});
                    }
                    break;
                }

                case LLVMLoad: {
                    LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);
                    int x = offsetMap.at(firstOperand);
                    offsetMap.insert({instruction, x});
                    break;
                }
            }
        }
    }
    return offsetMap;
}



// FIX THIS !!!!!!!

/***************** printDirectives ***********************/
void printDirectives(FILE* fp, LLVMValueRef function) 
{
    fprintf(fp, ".text\n");
    fprintf(fp, ".globl\n");
    fprintf(fp, ".type\n");
    fprintf(fp, "function\n");

    return;
}



/***************** printFunctionEnd ***********************/
void printFunctionEnd(FILE* fp)
{

    // Emitting leave and return instructions
    fprintf(fp, "leave\n");
    fprintf(fp, "ret\n");
}



/***************** createBBLabels ***********************/
unordered_map<LLVMBasicBlockRef, const char*> createBBLabels(LLVMValueRef function)
{
    // Initializing map
    unordered_map<LLVMBasicBlockRef, const char*> bbLabelMap;

    // Iterating through each basic block in the function
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL; 
            bb = LLVMGetNextBasicBlock(bb)) {

        // Adding basic block reference and name string to map
        const char* bbName = LLVMGetBasicBlockName(bb);
        bbLabelMap.insert({bb, bbName});
    }

    return bbLabelMap;
}




/***************** generateAssemblyCode ***********************/
void generateAssemblyCode(FILE* fp, LLVMValueRef function, unordered_map<LLVMValueRef, string> registerAssignments)
{
    // Creating BBLabels
    unordered_map<LLVMBasicBlockRef, const char*> bbLabelMap;
    bbLabelMap = createBBLabels(function);

    // Printing directives
    printDirectives(fp, function);

    // Initializing offset map
    int localMem = 0;
    unordered_map<LLVMValueRef, int> offsetMap = getOffsetMap(function, &localMem);

    

    // Pushing value of base pointer onto stack
    fprintf(fp, "pushl %%ebp\n");

    // Updating base pointer
    fprintf(fp, "movl %%esp, %%ebp\n");

    // Updating stack pointer
    fprintf(fp, "subl %d, %%esp\n", localMem);

    // Freeing register %ebx for new function 
    fprintf(fp, "pushl %%ebx\n");

    // Iterating through each basic block in the function
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL; 
            bb = LLVMGetNextBasicBlock(bb)) {

        // Printing basic block label
        const char* bbName = bbLabelMap.at(bb);
        fprintf(fp, "\n\nBasic block: %s\n\n", bbName);

        // Iterating through each instruction in the basic block
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction != NULL; 
                instruction = LLVMGetNextInstruction(instruction)) {

            LLVMOpcode instType = LLVMGetInstructionOpcode(instruction);
                
            switch(instType) {

                /* Case 1: Return instruction */
                case LLVMRet: {

                    // Extracting return value from LLVM return instruction
                    LLVMValueRef retValue = LLVMGetOperand(instruction, 0);

                    // If returning a constant
                    if (LLVMIsConstant(retValue)) {
                        long long retInt = LLVMConstIntGetSExtValue(retValue);
                        printf("TEST1\n");
                        fprintf(fp, "movl $%lld, %%eax", retInt);     
                    }

                    // Otherwise, if returning a temporary variable
                    else {
                        string assignedRegName = registerAssignments.at(retValue);

                        // If returning a temporary variable stored in memory, emitting appropriate instruction
                        if (strcmp(assignedRegName.c_str(), "-1") == 0) {
                            int k = offsetMap.at(retValue);
                            printf("TEST2\n");
                            fprintf(fp, "movl %d(%%ebp), %%eax\n", k);
                        }

                        // If returning a temporary variable stored in register, emitting appropriate instruction
                        else {
                            fprintf(fp, "movl %s, %%eax\n", assignedRegName.c_str());
                        }
                    }

                    // Retrieving value initially stored in register %ebx
                    fprintf(fp, "popl %%ebx\n");

                    // Printing function end
                    printFunctionEnd(fp);
                    break;
                }


                /* Case 2: Load instruction */
                case LLVMLoad: {

                    // Checking if register has been assigned to instruction
                    string assignedRegName = registerAssignments.at(instruction);

                    // If no register is assigned to instruction, retrieve variable value from stack
                    if (strcmp(assignedRegName.c_str(), "-1") != 0) {
                        int c = offsetMap.at(LLVMGetOperand(instruction, 0));
                        printf("TEST3\n");
                        fprintf(fp, "movl %d(%%ebp),%s\n", c, assignedRegName.c_str());
                    }
                    break;
                }


                /* Case 3: Store instruction */
                case LLVMStore: {

                    // Retrieving operand
                    LLVMValueRef operand = LLVMGetOperand(instruction, 0);

                    // Skipping if store instruction concerns a parameter
                    if (operand == LLVMGetParam(function, 0)) {
                        continue;
                    }

                    // If store instruction saves a constant
                    else if (LLVMIsConstant(operand)) {
                        long long intValue = LLVMConstIntGetSExtValue(operand);
                        int c = offsetMap.at(operand);
                        printf("TEST4\n");
                        fprintf(fp, "movl $%lld, %d(%%ebp)\n", intValue, c);
                    }

                    // If store instruction concerns a temporary variable
                    else {
                        string assignedRegName = registerAssignments.at(operand);

                        // If operand has been assigned a register
                        if (strcmp(assignedRegName.c_str(), "-1") != 0) {
                            int c = offsetMap.at(LLVMGetOperand(instruction, 1));
                            printf("TEST5\n");
                            fprintf(fp, "movl %s, %d(%%ebp)\n", assignedRegName.c_str(), c);
                        }

                        else {
                            int c1 = offsetMap.at(LLVMGetOperand(instruction, 0));
                            fprintf(fp, "movl %d(%%ebp), %%eax\n", c1);
                            int c2 = offsetMap.at(LLVMGetOperand(instruction, 1));
                            fprintf(fp, "movl %%eax, %d(%%ebp)\n", c2);
                        }
                    }
                    break;
                }


                /* Case 4: Call instruction */
                case LLVMCall: {

                    // Saving contents of register before function call
                    fprintf(fp, "pushl %%ecx\n");
                    fprintf(fp, "pushl %%edx\n");

                    // If function has a parameter
                    LLVMValueRef parameter;
                    if (LLVMGetNumArgOperands(instruction) > 0) {

                        // Extracting parameter
                        parameter = LLVMGetArgOperand(instruction, 0);

                        // If the parameter is a constant
                        if (LLVMIsConstant(parameter)) {
                            long long constVal = LLVMConstIntGetSExtValue(parameter);
                            fprintf(fp, "pushl $%lld\n", constVal);
                        }
                            
                        // If the parameter is a temporary variable
                        else {

                            // If the parameter has a register assigned
                            string assignedRegName = registerAssignments.at(parameter);
                            if (strcmp(assignedRegName.c_str(), "-1") != 0) {
                                fprintf(fp, "pushl %s\n", assignedRegName.c_str());
                            }
                                
                            else {
                                int k = offsetMap.at(parameter);
                                fprintf(fp, "pushl %d(%%ebp)\n", k);
                            }
                        }

                        // Emitting call function
                        if ((parameter = LLVMGetFirstParam(function)) != NULL) {

                            // Undoing the pushing of the parameter
                            fprintf(fp, "addl $4, %%esp\n");

                            // If instruction is of the form (%a = call type @func())
                            char* instructionString = LLVMPrintValueToString(instruction);
                            char firstChar = instructionString[0];

                            if (firstChar == '%') {

                                // If instruction has physical register assigned to it
                                if (strcmp(registerAssignments.at(instruction).c_str(), "-1") != 0) {

                                    string assignedRegName = registerAssignments.at(instruction);
                                    fprintf(fp, "movl %%eax, %s\n", assignedRegName.c_str());

                                    // Updating value in memory if applicable
                                    auto iter = offsetMap.find(instruction);
                                    if (iter != offsetMap.end()) {
                                        int k = offsetMap.at(instruction);

                                        fprintf(fp, "movl %%eax, %d(%%ebp)\n", k);
                                        fprintf(fp, "popl %%edx\n");
                                        fprintf(fp, "popl %%ecx\n");
                                    }
                                }
                            }
                        }
                    }
                    break;
                }


                /* Case 4: Branch instruction */
                case LLVMBr: {

                        int numOperands = LLVMGetNumOperands(instruction);

                        // If unconditional branch
                        if (numOperands == 1) {

                            // NOT SURE
                            LLVMBasicBlockRef block = LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 0));
                            
                            // Retrieving label of destination block
                            const char* newBlockName = bbLabelMap.at(block);

                            // Emitting jump instruction
                            fprintf(fp, "jmp %s\n", newBlockName);
                        }

                        // If conditional branch
                        else {

                            // Retrieving labels of basic blocks
                            const char* L1 = bbLabelMap.at(LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 2)));
                            const char* L2 = bbLabelMap.at(LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 1)));

                            // Retrieving predicate
                            LLVMIntPredicate jmpPred = LLVMGetICmpPredicate(LLVMGetOperand(instruction, 0));

                            // Based on predicate, generating appropriate jump instruction
                            switch(jmpPred) {

                                case LLVMIntEQ: {
                                    fprintf(fp, "je %s\n", L1);
                                    break;
                                }

                                case LLVMIntNE: {
                                    fprintf(fp, "jne %s\n", L1);
                                    break;
                                }

                                case LLVMIntSGT: {
                                    fprintf(fp, "jg %s\n", L1);
                                    break;
                                }

                                case LLVMIntSGE: {
                                    fprintf(fp, "jge %s\n", L1);
                                    break;
                                }

                                case LLVMIntSLT: {
                                    fprintf(fp, "jl %s\n", L1);
                                    break;
                                }

                                case LLVMIntSLE: {
                                    fprintf(fp, "jle %s\n", L1);
                                    break;
                                }
                            }

                            // Emitting L2 jump
                            fprintf(fp, "jmp %s\n", L2);
                        }
                    break;
                    }


                    /* Case 6: Arithmetic instructions */
                    case LLVMAdd: {
                        generateAssemblyArithmetic(fp, function, localMem, registerAssignments, instruction, offsetMap);
                        break;
                    }

                    case LLVMSub: {
                        generateAssemblyArithmetic(fp, function, localMem, registerAssignments, instruction, offsetMap);
                        break;
                    }

                    case LLVMMul: {
                        generateAssemblyArithmetic(fp, function, localMem, registerAssignments, instruction, offsetMap);
                        break;
                    }

                    /* Case 7: Comparison instruction */
                    case LLVMICmp: {
                        generateAssemblyCompare(fp, function, localMem, registerAssignments, instruction, offsetMap);
                        break;
                    }
                }
            }
        }
    }



/***************** generateAssemblyArithmetic ***********************/
void generateAssemblyArithmetic(FILE* fp, LLVMValueRef function, int localMem, 
        unordered_map<LLVMValueRef, string> registerAssignments, LLVMValueRef instruction,
        unordered_map<LLVMValueRef, int> offsetMap)
{
    // Checking if instruction is assigned to register
    string assignedRegName = registerAssignments.at(instruction);

    // If instruction has not been assigned to register, assigning it to %eax
    if (strcmp(assignedRegName.c_str(), "-1") == 0) {
        assignedRegName = "%eax";
    }

    // Extracting operands
    LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);
    LLVMValueRef secondOperand = LLVMGetOperand(instruction, 1);

    // If first operand is a constant, moving constant into assigned register
    if (LLVMIsConstant(LLVMGetOperand(instruction, 0))) {

        long long firstOpConst = LLVMConstIntGetSExtValue(firstOperand);
        fprintf(fp, "movl $%lld, %s\n", firstOpConst, assignedRegName.c_str());
    }

    // If first operand is a temporary variable
    else {

        // Checking for assigned register
        if (strcmp(registerAssignments.at(firstOperand).c_str(), "-1") != 0) {
            string operandAssignedRegName = registerAssignments.at(firstOperand);

            // Only emitting instruction if both registers are the same
            if (strcmp(operandAssignedRegName.c_str(), assignedRegName.c_str()) != 0) {
                fprintf(fp, "movl %s, %s\n", operandAssignedRegName.c_str(), assignedRegName.c_str());
            }
        }
        
        // If first operand is in memory
        else {
            int n = offsetMap.at(firstOperand);
            fprintf(fp, "movl %d(%%ebp), %s\n", n, assignedRegName.c_str());
        }
    }

    // Determining type of arithmetic operation
    LLVMOpcode arithType = LLVMGetInstructionOpcode(instruction);

    // If second operand is a constant
    if (LLVMIsConstant(LLVMGetOperand(instruction, 1))) {

        long long secondOpConst = LLVMConstIntGetSExtValue(secondOperand);

        switch (arithType) {
            case LLVMAdd: {
                fprintf(fp, "addl $%lld, %s\n", secondOpConst, assignedRegName.c_str());
                break;
            }
            case LLVMSub: {
                fprintf(fp, "subl $%lld, %s\n", secondOpConst, assignedRegName.c_str());
                break;
            }
            case LLVMMul: {
                fprintf(fp, "mul $%lld, %s\n", secondOpConst, assignedRegName.c_str());
                break;
            }
        }
        
    }

    // If second operand is a temporary variable
    else {

        string operandAssignedRegName = registerAssignments.at(secondOperand);

        // If second operand has already been assigned register
        if (strcmp(registerAssignments.at(secondOperand).c_str(), "-1") != 0) {

            if (strcmp(operandAssignedRegName.c_str(), assignedRegName.c_str()) != 0) {

                switch (arithType) {
                    case LLVMAdd: {
                        fprintf(fp, "addl %s, %s\n", operandAssignedRegName.c_str(), assignedRegName.c_str());
                        break;
                    }
                    case LLVMSub: {
                        fprintf(fp, "subl %s, %s\n", operandAssignedRegName.c_str(), assignedRegName.c_str());
                        break;
                    }
                    case LLVMMul: {
                        fprintf(fp, "mul %s, %s\n", operandAssignedRegName.c_str(), assignedRegName.c_str());
                        break;
                    }
                }
            }
        }
    
        // If second operand has not been assigned register
        else {

            int m_1 = offsetMap.at(secondOperand);

            switch (arithType) {
                case LLVMAdd: {
                    fprintf(fp, "addl %d(%%ebp), %s\n", m_1, assignedRegName.c_str());
                    break;
                }
                case LLVMSub: {
                    fprintf(fp, "subl %d(%%ebp), %s\n", m_1, assignedRegName.c_str());
                    break;
                }
                case LLVMMul: {
                    fprintf(fp, "mul %d(%%ebp), %s\n", m_1, assignedRegName.c_str());
                    break;
                }
            }
        }
    }

    // If result is stored in memory (if it is not a temporary variable), updating it

    // ASK: WHAT IF RESULT NOT STORED IN EAX BUT ANOTHER REGISTER
    auto iter = offsetMap.find(instruction);
    if (iter != offsetMap.end()) {

        int k = offsetMap.at(instruction);
        fprintf(fp, "movl %%eax, %d(%%ebp)\n", k);
    }
}

    

/***************** generateAssemblyCompare ***********************/
void generateAssemblyCompare(FILE* fp, LLVMValueRef function, int localMem, 
        unordered_map<LLVMValueRef, string> registerAssignments, LLVMValueRef instruction,
        unordered_map<LLVMValueRef, int> offsetMap)
{
    // Checking if instruction is assigned to register
    string assignedRegName = registerAssignments.at(instruction);

    // If instruction has not been assigned to register, assigning it to %eax
    if (strcmp(assignedRegName.c_str(), "-1") == 0) {
        assignedRegName = "%eax";
    }

    // Extracting operands
    LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);
    LLVMValueRef secondOperand = LLVMGetOperand(instruction, 1);

    // If first operand is a constant, moving constant into assigned register
    if (LLVMIsConstant(LLVMGetOperand(instruction, 0))) {

        long long firstOpConst = LLVMConstIntGetSExtValue(firstOperand);
        fprintf(fp, "movl $%lld, %s\n", firstOpConst, assignedRegName.c_str());
    }

    // If first operand is a temporary variable
    else {

        // Checking for assigned register
        if (strcmp(registerAssignments.at(firstOperand).c_str(), "-1") != 0) {
            string operandAssignedRegName = registerAssignments.at(firstOperand);

            // Only emitting instruction if both registers are the same
            if (strcmp(operandAssignedRegName.c_str(), assignedRegName.c_str()) != 0) {
                fprintf(fp, "movl %s, %s\n", operandAssignedRegName.c_str(), assignedRegName.c_str());
            }
        }
        
        // If first operand is in memory
        else {
            int n = offsetMap.at(firstOperand);
            fprintf(fp, "movl %d(%%ebp), %s\n", n, assignedRegName.c_str());
        }
    }

    // If second operand is a constant
    if (LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
        long long secondOpConst = LLVMConstIntGetSExtValue(secondOperand);
        fprintf(fp, "cmpl $%lld, %s\n", secondOpConst, assignedRegName.c_str());
    }

    // If second operand is a temporary variable
    else {

        string operandAssignedRegName = registerAssignments.at(secondOperand);

        // If second operand has already been assigned register
        if (strcmp(registerAssignments.at(secondOperand).c_str(), "-1") != 0) {
            fprintf(fp, "cmpl %s, %s\n", operandAssignedRegName.c_str(), assignedRegName.c_str());
        }
    
        // If second operand has not been assigned register
        else {
            int m_1 = offsetMap.at(secondOperand);
            fprintf(fp, "cmpl %d(%%ebp), %s\n", m_1, assignedRegName.c_str());
        }
    }
}