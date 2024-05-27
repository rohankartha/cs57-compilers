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





void computeLiveness(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, int>* instIndex, unordered_map<LLVMValueRef, array<int, 2>>* liveRange);








// anything that involves register?
// look at liveness of each temporary variable









/***************** computeLiveness ***********************/
void computeLiveness(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, int>* instIndex, unordered_map<LLVMValueRef, array<int, 2>>* liveRange)
{

    int index = -1;

    // Iterating through instructions in basic block
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst)) {
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




            //liveRange->insert({inst, newRangeVec});









            bool use = false;
            int endIndex = index;
            int indexCopy = index;

            LLVMValueRef nextInst = LLVMGetNextInstruction(inst);

            for (LLVMValueRef checkInst = nextInst; 
                    checkInst != NULL; checkInst = LLVMGetNextInstruction(checkInst)) {

                indexCopy++;
                int numOperands = LLVMGetNumOperands(nextInst);


                for (int i = 0; i < numOperands; i++) {
                    LLVMValueRef operand = LLVMGetOperand(checkInst, i);


                    if (inst == operand) {
                        use = true;
                        endIndex = indexCopy;
                    }
                }



            // int indexCopy = index;
            // LLVMValueRef nextInst = inst;
            // int endIndex = 0;
            


            // while ((nextInst = LLVMGetNextInstruction(nextInst)) != NULL) {

            //     indexCopy++;

            //     // for loop num of operands in instruction
            //     int numOperands = LLVMGetNumOperands(nextInst);
            //     for (int i = 0; i < numOperands; i++) {
            //         LLVMValueRef operand = LLVMGetOperand(inst, i);

            //         // If operand is same as instruction, updating end index
            //         if (inst = operand) {
            //             use = true;
            //             endIndex = indexCopy;
            //         }
            //     }    
            // }

            // auto instEntry = liveRange->find(inst);
            // vector<int> indices = instEntry->second;
            // indices.push_back(endIndex);
            // instEntry->second = indices;
        }

        // auto instEntry = liveRange->find(inst);
        // instEntry->second.push_back(endIndex);
        range[1] = endIndex;

        liveRange->insert({inst, range});



    }
}
}






// /***************** allocateRegisters ***********************/
// unordered_map<LLVMValueRef, string> allocateRegisters(LLVMValueRef function) 
// {

//     unordered_map<LLVMValueRef, string> registerAssignments;

//     for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); 
//             bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {

//         // Initializing set of available registers
//         set<string> availableRegisters;

//         // Adding %ebx, %edx, and %ecx to set
//         availableRegisters.insert(string("ebx"));
//         availableRegisters.insert(string("edx"));
//         availableRegisters.insert(string("ecx"));

//         // Calculating index map and range map
//         unordered_map<LLVMValueRef, int> instIndex;
//         unordered_map<LLVMValueRef, vector<int>> liveRange;
//         computeLiveness(bb, instIndex, liveRange);

//         int instructionNum = 0;

//         // Iterating through instructions in basic block
//         for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb);    
//                 instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
            
//             LLVMOpcode instructionType = LLVMGetInstructionOpcode(instruction);
//             instructionNum++;

//             // If instruction is an allocate instruction, skip
//             if (LLVMGetInstructionOpcode(instruction) == LLVMAlloca) {
//                 continue;
//             }

            
//             // If instruction doesn't have a result
//             else if () {

//                 // If any of the operands' liveness range ends at this instruction
//                 int numOperands = LLVMGetNumOperands(instruction);
//                 for (int i = 0; i < numOperands; i++) {

//                     vector<int> operandLiveRange = liveRange.at(instruction); 
//                     int endIndex = operandLiveRange.at(1);

//                     if (endIndex == instructionNum) {

//                         // Adding register to availableRegisters set
//                         LLVMValueRef operand = LLVMGetOperand(instruction, i);

//                         string registerName = registerAssignments.at(operand);
//                         availableRegisters.insert(registerName);

//                     }
//                 }

//             }

//             else {

//                 if (instructionType == LLVMAdd || instructionType == LLVMSub || instructionType == LLVMMul) {

//                     if (registerAssignments.find(instruction) != registerAssignments.end()) {

//                         string registerName = registerAssignments.find(instruction)->second;

//                         vector<int> operandRange = liveRange.at(LLVMGetOperand(instruction, 0));

//                         if (operandRange.at(1) == instructionNum) {
//                             registerAssignments.insert({instruction, registerName});

//                             LLVMValueRef operandTwo = LLVMGetOperand(instruction, 1);
//                             vector<int> operandTwoRange = liveRange.at(operandTwo);

//                             int operandTwoEndIndex = operandTwoRange.at(1);

//                             if (operandTwoEndIndex == instructionNum) {
                                
//                                 if (registerAssignments.find(operandTwo) == registerAssignments.end()) {
//                                     string registerName = registerAssignments.at(operandTwo);

//                                     availableRegisters.insert(registerName);
//                                 }
//                             }


//                         }

                        

//                     }
//                 }

//                 // If a physical register is available
//                 else if (!availableRegisters.empty()) {

//                     // Retrieving available register name
//                     string availableRegName = *availableRegisters.begin();

//                     // Removing register name from available registers and adding it to assignment map
//                     registerAssignments.insert({instruction, availableRegName});
//                     availableRegisters.erase(availableRegName);

//                     // Marking any registers used by operands at the end of their live ranges as available
//                     for (int i = 0; i < LLVMGetNumOperands(instruction); i++) {

//                         LLVMValueRef operand = LLVMGetOperand(instruction, i);
//                         int operandEndIndex = liveRange.at(operand).at(1);

//                         if (operandEndIndex == instructionNum) {
//                             string newlyAvailableReg = registerAssignments.at(operand);
//                             availableRegisters.insert(newlyAvailableReg);
//                         }
//                     }
//                 }

//                 // If a physical register is not available
//                 else {
//                     LLVMValueRef V = findSpill(registerAssignments, liveRange, sortedList, instruction);

//                     int v_LivenessRange = liveRange.at(V).at(1);
//                     int inst_LivenessRange = liveRange.at(instruction).at(1);

//                     if (inst_LivenessRange > v_LivenessRange) {
//                         registerAssignments.insert({instruction, "-1"});
//                     }

//                     else {
//                         string registerName = registerAssignments.at(V);
//                         registerAssignments.insert({instruction, registerName});

//                         // Does this update???
//                         registerAssignments.insert({V, "-1"});
//                     }

//                     // Marking any registers used by operands at the end of their live ranges as available
//                     for (int i = 0; i < LLVMGetNumOperands(instruction); i++) {

//                         LLVMValueRef operand = LLVMGetOperand(instruction, i);
//                         int operandEndIndex = liveRange.at(operand).at(1);

//                         if (operandEndIndex == instructionNum) {
//                             string newlyAvailableReg = registerAssignments.at(operand);
//                             availableRegisters.insert(newlyAvailableReg);
//                         }
//                     }
//                 }
//             }





//             // If instruction does have a result

//         }




//     }





// } 


// /***************** findSpill ***********************/
// LLVMValueRef findSpill(unordered_map<LLVMValueRef, string> registerAssignments, 
//         unordered_map<LLVMValueRef, vector<int>> liveRange, vector<LLVMValueRef> sortedList,
//         LLVMValueRef instruction)
// {

//     auto instIter = find(sortedList.begin(), sortedList.end(), instruction);
//     int instIndex = distance(sortedList.begin(), instIter);


//     for (int i = 0; sortedList.size(); i++) {
//         LLVMValueRef instructionTwo = sortedList.at(i);

//         int firstInstEnd;
//         int secondInstBeg;

//         // Determining relative order of two instructions
//         if (i > instIndex) {
//             firstInstEnd = liveRange.at(instructionTwo).at(1);
//             secondInstBeg = liveRange.at(instruction).at(0);
//         }
//         else {
//             firstInstEnd = liveRange.at(instruction).at(1);
//             secondInstBeg = liveRange.at(instructionTwo).at(0);
//         }

//         // If the liveness ranges overlap
//         if (secondInstBeg >= firstInstEnd) {
//             if (registerAssignments.find(instructionTwo) != registerAssignments.end()) {
//                 string registerName = registerAssignments.at(instructionTwo);

//                 if (registerName == "-1") {
//                     return instructionTwo;
//                 }
//             }

//         }
        
        



//         // a_end >= b_beginninf
//     }
//     return NULL;
// }





// /***************** getOffsetMap ***********************/
// unordered_map<LLVMValueRef, int> getOffsetMap(LLVMModuleRef m)
// {
//     // Initializing localMem (represents offset from %ebp)
//     int localMem = 4;

//     // Intializing offsetMap
//     unordered_map<LLVMValueRef, int> offsetMap;

//     LLVMValueRef function = LLVMGetFirstFunction(m);
//     LLVMValueRef funcParam = LLVMGetParam(function, 0);

//     // Generating instruction to push parameter onto stack
//     if (funcParam != NULL) {
//         offsetMap.insert({funcParam, 8});
//     }

//     // Iterating through each basic block in the function
//     for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL; 
//             bb = LLVMGetNextBasicBlock(bb)) {
        
//         // Iterating through each instruction in the basic block
//         for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction != NULL;
//                 instruction = LLVMGetNextInstruction(instruction)) {

//             LLVMOpcode instructionType = LLVMGetInstructionOpcode(instruction);

//             switch (instructionType) {

//                 // If instruction is an allocate instruction
//                 case LLVMAlloca: {
//                     localMem = localMem + 4;
//                     offsetMap.insert({instruction, -1*localMem});

//                 }

//                 // If instruction is a store instruction
//                 case LLVMStore: {

//                     // If instruction stores parameter value, set offset position for second operand as equal to same value
//                     LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);
//                     LLVMValueRef secondOperand = LLVMGetOperand(instruction, 1);

//                     if (firstOperand == funcParam) {
//                         int x = offsetMap.at(firstOperand);
//                         offsetMap.insert({secondOperand, x});
//                     }

//                     // If instruction does not store parameter value
//                     else {
//                         int x = offsetMap.at(secondOperand);
//                         offsetMap.insert({firstOperand, x});
//                     }
//                 }

//                 case LLVMLoad: {
//                     LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);
//                     int x = offsetMap.at(firstOperand);
//                     offsetMap.insert({instruction, x});

//                 }

                
//             }




//         }
        

                
        

//     }


// }


// /***************** printDirectives ***********************/
// void printDirectives(FILE* fp, char* assemblyDir) 
// {
//     fprintf(fp, assemblyDir);
// }

// /***************** printDirectives ***********************/
// void printAssemblyInstruction(FILE* fp, char* assemblyInst) 
// {
//     fprintf(fp, assemblyInst);
// }




// /***************** printFunctionEnd ***********************/



// /***************** generateAssemblyCode ***********************/
// void generateAssemblyCode(FILE* fp, LLVMModuleRef m, int localMem, unordered_map<LLVMValueRef, string> registerAssignments)
// {

//     // Iterating through functions in module
//     for (LLVMValueRef function = LLVMGetFirstFunction(m); function != NULL; 
//             function = LLVMGetNextFunction(function))
//     {

//         // Printing directives


//         // Initializing offset map
//         unordered_map<LLVMValueRef, int> offsetMap = getOffsetMap(m);

//         // Pushing value of base pointer onto stack
//         fprintf(fp, "pushl %%ebp");

//         // Updating base pointer
//         fprintf(fp, "movl %%esp, %%ebp");

//         // Updating stack pointer
//         fprintf(fp, "subl %d, %%esp", localMem);

//         // Freeing register %ebx for new function 
//         fprintf(fp, "pushl %%ebx");

//         // Iterating through each basic block in the function
//         for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != NULL; 
//                 bb = LLVMGetNextBasicBlock(bb)) {
            
//             // Iterating through each instruction in the basic block
//             for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction != NULL; 
//                     instruction = LLVMGetNextInstruction(instruction)) {

//                 LLVMOpcode instType = LLVMGetInstructionOpcode(instruction);

//                 switch(instType) {
//                     case LLVMRet: {
                        
//                         // Extracting return value from LLVM return instruction
//                         LLVMValueRef retValue = LLVMGetOperand(instruction, 0);

//                         // If returning a constant
//                         if (LLVMIsConstant(retValue)) {

//                             long long retInt = LLVMConstIntGetSExtValue(retValue);
//                             fprintf(fp, "movl $%d, %%eax", retInt);
                            
//                         }

                        
//                         else {

//                             string assignedRegName = registerAssignments.at(retValue);

//                             // If returning a temporary variable stored in memory, emitting appropriate instruction
//                             if (strcmp(assignedRegName.c_str(), "-1") == 0) {

//                                 int k = offsetMap.at(retValue);
//                                 fprintf(fp, "movl %d(%%ebp), %%eax\n", k);
//                             }

//                             // If not returning a temporary variable stored in register, emitting appropriate instruction
//                             else {
//                                 fprintf(fp, "movl %s, %%eax", assignedRegName.c_str());
//                             }
//                         }

//                         // Retrieving value initially stored in register %ebx
//                         fprintf(fp, "popl %%ebx\n");

//                         // Printing function end
//                     }

//                     case LLVMLoad: {

//                         // Checking if register has been assignment to instruction
//                         string assignedRegName = registerAssignments.at(instruction);


//                         // ????
//                         if (!assignedRegName.empty()) {
//                             int c = offsetMap.at(LLVMGetOperand(instruction, 1));
//                             fprintf(fp, "movl %d(%%ebp),%%s", c, assignedRegName);
//                         }
//                     }






//                     case LLVMStore: {

//                         // Skipping if store instruction concerns a parameter
//                         LLVMValueRef operand = LLVMGetOperand(instruction, 0);
//                         if (operand == LLVMGetParam(function, 0)) {
//                             return;
//                         }

//                         // If store instruction concerns a constant
//                         else if (LLVMConstIntGetSExtValue(operand)) {
//                             long long intValue = LLVMConstIntGetSExtValue(operand);
//                             int c = offsetMap.at(operand);
//                             fprintf(fp, "movl $%d, %d(%%ebp)\n", intValue, c);
//                         }

//                         // If store instruction concerns a temporary variable
//                         else {
//                             string assignedRegName = registerAssignments.at(operand);

//                             if (strcmp(assignedRegName.c_str(), "-1") != 0) {

                                
//                                 int c = offsetMap.at(LLVMGetOperand(instruction, 1));
//                                 fprintf(fp, "movl %s, %d(%%ebp)", assignedRegName, c);
//                             }

//                             else {
//                                 int c1 = offsetMap.at(LLVMGetOperand(instruction, 0));
//                                 fprintf(fp, "movl %d(%%ebp), %%eax", c1);
//                                 int c2 = offsetMap.at(LLVMGetOperand(instruction, 1));
//                                 fprintf(fp, "movl %%eax, %d(%%ebp)", c2);
//                             }
//                         }
//                     }







//                     case LLVMCall: {

//                         // Saving contents of register before function call
//                         fprintf(fp, "pushl %%ecx");
//                         fprintf(fp, "pushl %%edx");

//                         // If function has a parameter
//                         LLVMValueRef parameter;
//                         if ((parameter = LLVMGetFirstParam(function)) != NULL) {
                            
//                             // If the parameter is a constant
//                             if (LLVMIsConstant(parameter)) {
//                                 long long constVal = LLVMConstIntGetSExtValue(parameter);
//                                 fprintf(fp, "pushl $%d", constVal);
//                             }
                            
//                             // If the parameter is a temporary variable
//                             else {

//                                 string assignedRegName = registerAssignments.at(parameter);
//                                 // ????
//                                 if (!assignedRegName.empty()) {
//                                     fprintf(fp, "pushl %s", assignedRegName);

//                                 }
                                
//                                 else {
//                                     int k = offsetMap.at(parameter);
//                                     fprintf(fp, "pushl %d(%%ebp)", k);
//                                 }

//                             }

//                             // Emitting call function

//                             /*
//                             emit call func
// if func has a param P
// emit addl $4, %esp (this is to undo the pushing of the parameter)
// if Instr is of the form (%a = call type @func())
// if %a has a physical register %exx assigned to it
// emit movl %eax, %exx
// if %a is in memory
// get offset k of %a
// emit movl %eax, k(%ebp)
// emit popl %edx
// emit popl %ecx*/

//                         }






//                     }



//                     case LLVMBr: {
//                         int numOperands = LLVMGetNumOperands(instruction);

//                         /*
//                         if the branch is unconditional (br label %b)
// get label L of %b from bb_labels
// emit jmp L
// if the branch is conditional (br i1 %a, label %b, label %c)
// get labels L1 for %b and L2 for %c from bb_labels
// get the predicate T of comparison from instruction %a (<, >, <=, >=, ==)
// based on value of T:
// emit jxx L1 [replace jxx with conditional jump assembly instruction corresponding to T]
// emit jmp L2 */

//                         // If unconditional branch
//                         if (numOperands == 0) {

//                         }

//                         // If conditional branch
//                         else {

//                         }





//                     }

//                     case LLVMAdd: {}

//                     case LLVMSub: {}

//                     case LLVMMul: {}

//                     // WRITE THIS HELPER FUNCTION
//                     case LLVMICmp: {}
//                 }

                

//             }

//         }











//     }



// }

// void generateAssemblyArithmetic(FILE* fp, LLVMModuleRef m, int localMem, 
//         unordered_map<LLVMValueRef, string> registerAssignments, LLVMValueRef instruction,
//         unordered_map<LLVMValueRef, int> offsetMap)
// {

//     // Checking if instruction is assigned to register
//     string assignedRegName = registerAssignments.at(instruction);

//     // ??
//     if (assignedRegName.empty()) {
//         assignedRegName = "%%eax";
//     }

//     // Extracting operands
//     LLVMValueRef firstOperand = LLVMGetOperand(instruction, 0);
//     LLVMValueRef secondOperand = LLVMGetOperand(instruction, 1);


//     // If first operand is a constant, moving constant into assigned register
//     if (LLVMIsConstant(LLVMGetOperand(instruction, 0))) {

//         long long firstOpConst = LLVMConstIntGetSExtValue(firstOperand);
//         fprintf(fp, "movl $%d, %s", firstOpConst, assignedRegName);
//     }

//     // If first operand is a temporary variable
//     else {

//         // Checking for assigned register
//         // ??
//         if (!registerAssignments.at(firstOperand).empty()) {
//             string operandAssignedRegName = registerAssignments.at(firstOperand);

//             if (strcmp(operandAssignedRegName.c_str(), assignedRegName.c_str()) != 0) {
//                 fprintf(fp, "movl %s, %s", operandAssignedRegName, assignedRegName);
//             }

//         }
        
//         // If first operand is in memory
//         // update register map here?
//         // add \ instead of second %?
//         else {
//             int n = offsetMap.at(firstOperand);
//             fprintf(fp, "movl %d(%%ebp), %s", n, assignedRegName);
//         }
//     }

    

//     // If second operand is a constant
//     if (LLVMIsConstant(LLVMGetOperand(instruction, 1))) {

//         long long secondOpConst = LLVMConstIntGetSExtValue(secondOperand);
//         LLVMOpcode arithType = LLVMGetInstructionOpcode(instruction);

//         switch (arithType) {
//             case LLVMAdd: {
//                 fprintf(fp, "addl $%d, %s", secondOpConst, assignedRegName);
//             }
//             case LLVMSub: {
//                 fprintf(fp, "subl $%d, %s", secondOpConst, assignedRegName);
//             }
//             case LLVMMul: {
//                 fprintf(fp, "mul $%d, %s", secondOpConst, assignedRegName);
//             }
//         }
        
//     }

//     // If second operand is a temporary variable
//     else {
//         if (!registerAssignments.at(firstOperand).empty()) {
//             string operandAssignedRegName = registerAssignments.at(secondOperand);
//             LLVMOpcode arithType = LLVMGetInstructionOpcode(instruction);

//             if (strcmp(operandAssignedRegName.c_str(), assignedRegName.c_str()) != 0) {

//                 switch (arithType) {
//                     case LLVMAdd: {
//                         fprintf(fp, "addl %d(%%ebp), %s", operandAssignedRegName, assignedRegName);
//                     }
//                     case LLVMSub: {
//                         fprintf(fp, "subl %d(%%ebp), %s", operandAssignedRegName, assignedRegName);
//                     }
//                     case LLVMMul: {
//                         fprintf(fp, "mul %d(%%ebp), %s", operandAssignedRegName, assignedRegName);
//                     }
//                 }
    
//         }
//         else {

//             int m = offsetMap.at(secondOperand);

//             switch (arithType) {
//                 case LLVMAdd: {
//                     fprintf(fp, "addl %d(%%ebp), %s", operandAssignedRegName, assignedRegName);
//                 }
//                 case LLVMSub: {
//                     fprintf(fp, "subl %d(%%ebp), %s", operandAssignedRegName, assignedRegName);
//                 }
//                 case LLVMMul: {
//                     fprintf(fp, "mul %d(%%ebp), %s", operandAssignedRegName, assignedRegName);
//                 }
//             }
//         }

//         }

//     }

//     // If result is stored in memory (if it is a PARAMETER?)
//     /**if %a is in memory
// get offset k of %a
// emit movl %eax, k(%ebp)*/





// }





                                                       


