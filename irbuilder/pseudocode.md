# Pseudocode #

### addVarToMap ###

```none
if key with variable name does not exist in variableMap
            add variable to variableMap
        else
            if variable name does not exist in nameFrequency
                insert new pair of {variable name, 1}
                build new variable name string: "variable name" + "1" -> "variablename1"
                add "variablename1" to variableMap
            else
                increment current integer value assigned to variable name by 1
                build new variable name string: "variable name" + "n" -> "variablenamen"
                add "variablenamen" to variableMap
```

### createMap ###

```none
initialize map to hold variable names and alloc instructions (variableMap)
initialize map which maps variable names to integers (nameFrequency)
visit program node
visit function node
visit parameter node
Add parameter variable to map
visit block node
if block node has child "declarations"
    for each declaration in "declarations"
        visit declaration node 
        extract variable name from node
for each statement in block node child "statements" ***checkpoint 1***
    if statement is "return" statement
        visit child node of "return" statement 
        if child node is "var" node 
            addVarToMap
        if child node is "expr" node 
            visit children nodes of "expr" node
            if child node is "var" node 
                addVarToMap
    if statement is "assignment" statement
        visit "lhs" node
        if "lhs" node is "var" node
            addVarToMap
        visit "rhs" node
        if "rhs" node is "var" node
            addVarToMap
        else 
            if "rhs" node is "bexpr" node
                visit children nodes of "bexpr" node
                    if child node is "var" node
                        addVarToMap
    if statement is "if" assignment statement
        visit "if" node
        visit "condition" node child of "if" node 
        visit "lhs" child of "condition" node
        if "lhs" child is "var" node
            addVarToMap
        visit "rhs" child of "condition" node
        if "rhs" child is "var" node
            addVarToMap 
        visit "if_body" node child of "if" node
        if "if_body" node is "block" node
            conduct process started at *checkpoint 1* for this block
        else
            conduct process started at *checkpoint 1* for single statement
    if statement is "if-else" statement
        visit "if" node
        visit "condition" node child of "if" node 
        visit "lhs" child of "condition" node
        if "lhs" child is "var" node
            addVarToMap
        visit "rhs" child of "condition" node
        if "rhs" child is "var" node
            addVarToMap 
        visit "if_body" node child of "if" node
        if "if_body" node is "block" node
            conduct process started at *checkpoint 1* for this block
        else
            conduct process started at *checkpoint 1* for single statement
        visit "else" body node child of "if" node
            conduct process started at *checkpoint 1* for single statement
    if statement is "while" statement 
        visit "while" node
        visit "condition" node child of "while" node 
        visit "lhs" child of "condition" node
        if "lhs" child is "var" node
            addVarToMap
        visit "rhs" child of "condition" node
        if "rhs" child is "var" node
            addVarToMap 
        visit "body" node child of "while" node
        if "body" node is "block" node
            conduct process started at *checkpoint 1* for this block
        else
            conduct process started at *checkpoint 1* for single statement
    if statement is "call" statement
        if name is "print"
        visit "var" node child of "call" node
        addVarToMap
```

### Build IR ###

```none
create first basic block of the function
for each key in variableMap
    create alloc instruction
    add alloc instruction to variableMap as value for key
visit program node
visit function node
visit parameter node
create an alloc instruction for parameter
add parameter and alloc instruction to map
visit block node
if block node has child "declarations" 
    for each declaration in declarations
        Visit declaration node
        create an alloc instruction for declaration
        add variable and alloc instruction to map
for each statement in block node child "statements" ***checkpoint 2***
    if statement is "return" statement
        if child node is "var" node
            create alloc instruction and save to local variable
            create store instruction with alloc instruction and constant as operands
        if child node is "const" node
            create alloc instruction and save to local variable
            create store instruction with alloc instruction and constant as operands
        if child node is "bexpr" node
            create add/sub/mul instruction
            create alloc instruction and save to local variable
            create store instruction with alloc instruction and add/sub/mul as operands
    if statement is "assignment" statement
        visit "rhs" node
        if "rhs" node is "const" node
            create load instruction with constant as operand
        else if "rhs" is "var" node 
            create load instruction with register holding variable as operand + add to basic block
        else if "rhs" is "bexpr" node
            visit "lhs" child node of "bexpr" node
            if "lhs" child is a "var" node
                create store instruction with register holding variable as operand + add to basic block
            visit "rhs" child node of "bexpr" node
            if "rhs" child is a "var" node
                create load instruction with register holding variable as operand + add to basic block
            else if "rhs" child is a "const" node 
                create load instruction with register holding constant as operand + add to basic block
        create add/sub/mult instruction with load instruction registers as operands + add to basic block
    if statement is "if" statement
        create new basic block
        create branch instruction w/ new basic block name as operand and add it to active basic block
        mark new basic block as active basic block
        visit "if" node
        visit "condition" node child of "if" node
        if "lhs" and/or "rhs" node child of "condition" is a "var" node
            create load instruction for "var" + add it to active basic block
        create integer comparison instruction for condition + add it to block
        create new basic block for "if" body
        create branch instruction w/ new "if" body basic block name as operand + add it to active block
        mark new basic block as active basic block
        visit "if_body" node child of "if" node
        if "if_body" node is "block" node
            conduct process started at *checkpoint 2* for this block
        else
            conduct process started at *checkpoint 2* for single statement
        create new basic block (will be used as exit basic block)
        create new branch instruction w/ exit basic block name as operand  + add it to active basic block
    if statement is "if-else" statement
        create new basic block
        create branch instruction w/ new basic block name as operand and add it to active basic block
        mark new basic block as active basic block
        visit "if" node
        visit "condition" node child of "if" node
        if "lhs" and/or "rhs" node child of "condition" is a "var" node
            create load instruction for "var" + add it to active basic block
        create integer comparison instruction for condition + add it to block
        create new basic block for "if" body
        create new basic block for "else" body
        create branch instruction w/ new "if" body basic block name and new "else" basic block name as operands
        add this instruction to to active block
        mark new "if body" basic block as active basic block
        visit "if_body" node child of "if" node
        if "if_body" node is "block" node
            conduct process started at *checkpoint 2* for this block
        else
            conduct process started at *checkpoint 2* for single statement
        visit "else_body" node child of "if" node
        mark new "else body" basic block as active basic block
        if "else_body" node is "block" node
            conduct process started at *checkpoint 2* for this block
        else
            conduct process started at *checkpoint 2* for single statement
        create new basic block (will be used as exit basic block)
        create new branch instruction w/ exit basic block name as operand
        add this new instruction to "else body" and "if body" basic blocks
    if statement is "while" statement 
        create new basic block
        create branch instruction w/ new basic block name as operand and add it to active basic block
        mark new basic block as active basic block
        visit "while" node
        visit "condition" node child of "while" node
        if "lhs" and/or "rhs" node child of "condition" is a "var" node
            create load instruction for "var" + add it to active basic block
        create integer comparison instruction for condition + add it to block
        create new basic block for "while" body
        create branch instruction w/ new "while" body basic block name as operand + add it to active block
        mark new basic block as active basic block
        visit "while_body" node child of "while" node
        if "while_body" node is "block" node
            conduct process started at *checkpoint 2* for this block
        else
            conduct process started at *checkpoint 2* for single statement
        create new basic block (will be used as exit basic block)
        create new branch instruction w/ exit basic block name as operand  + add it to active basic block
    if statement is "call" statement
        if name is "print"
            extract variable name from node and alloc from variableMap
            create load instruction with variable alloc ref as operand
            create call instruction with register updated by previous instruction as operand
create final basic block 
create load instruction with operand from locally-saved memory location
```
