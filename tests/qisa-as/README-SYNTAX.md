# Assembly Syntax

This document describes the syntax of the programs that are accepted by
the QISA assembler.

### General

- All characters in the assembly code are case insensitive.
- Comments start with a hash mark (*#*) and continue to the end of the
  line.
- A line can be an empty line, or an instruction line.
  + An empty line is a line with only whitespace, which can be spaces,
    tabs, and/or comments.

#### Immediate Values
The immediate values (integers) can be specified in three different ways:
- Plain format: these are interpreted as base-10 numbers.
- Hexadecimal format: These values start with prefix `0x` and are
  interpreted as hexadecimal numbers.
- Binary format: These values start with prefix `0b` and are interpreted as
  binary numbers.

Actually, there is yet another way to specify immediate values:
- Symbols. These are named constants that have been defined using the
  assembler directive `.def_sym`, which is described below.


### Instruction types
An assembly program can contain two types of instructions:
- Single format, also known as classic instructions.
- Bundle format, which are exclusively used for quantum instructions.

Besides the instructions, a program can also contain assembler directives,
that are usually placed at the top of a program.
The assembler directives are described below.

### Registers
There are 4 types of registers that can be used in a program:

- The 'R' registers are used for the single format instructions.
- The 'Q' registers are used to contain results of quantum measurements.
- The 'S' registers are used to mark targets for single qubit quantum operations.
- The 'T' registers are used to mark control/target pairs for two qubit quantum operations.

### Classic instructions
The single-format or classic instructions are those instructions that
encode into a binary output 'word'.
The [Wiki](https://github.com/DiCarloLab-Delft/ElecPrj_CCLight/wiki)
contains several pages that describe some of these instructions in detail.

### Quantum instructions
Quantum instructions are specified in a special way called the _Bundle_ format.
These instructions are encoded such that two quantum instructions fit in one binary output 'word'.
This assembler defines three types of quantum instructions, but leaves the name and opcode up to the user.
The three types are:
- *arg\_none*: Used for the quantum instructions that do not use any arguments.
- *arg\_st*: Used for the single qubit quantum instructions that specify one 'S' register.
- *arg\_tt*: Used for qubit pair operations that specify one 'T' register.

**Note**: The _arg\_st_ instructions can be marked as *conditional*, by prefixing the instruction name by `C,`.


The names and opcodes are specified using a configuration file named `qisa_instructions.dbpd`.
This configuration file contains a description of its contents.

The quantum instructions are specified in the _Bundle_ format as follows:

\[`BS`\] \[\<Wait\_Time\>\] \<Quantum instruction\> \[`|` \<Quantum instruction\> \]*

The optional `BS` keyword can be used to stress the fact that a quantum bundle is being specified.
It has no effect and is ignored by the assembler.
The _Wait\_Time_ specifies the number of quantum instruction cycles to wait before executing the specified instructions.
If the _Wait\_Time_ is not specified, a default of 1 is used.
If the _Wait\_Time_ is 0, it depends whether the preceding instruction was a quantum instruction or a classic instruction.
If it was a quantum instruction, the instruction(s) following the 0 are appended to the bundle.
If it was not a quantum instruction, a new bundle is started and a waiting time of 0 quantum instruction cycles will be applied.


### Assembler directives

#### Register aliases

Registers can be given more meaningful names using the `.register` keyword.
Usage:

```
.register <Register Name> <Alias Name>
```

**Note**: you can not use a register name or an instruction name for an alias name.
For example: `.register s0 q0` and `.register s0 cmp` are illegal.
Exception to this rule are the quantum instructions, of which the instruction names (such as `X`) are allowed to be used as alias name.
Be careful though, to keep the meaning of the instructions in which these aliases are used clear.


#### Symbol definitions

Just as with registers, you can give numerical constants a more meaningful name.
To do this, you can use the `.def_sym` keyword.
Usage:

```
.def_sym <Alias Name> <Integer>
```

**Note**: The restrictions on the use of alias names apply for the symbol definitions as well.

### High-Level vs Low-Level instructions.

Some classic instructions are actually shorthands that combine multiple primitive (Low-Level) instructions.
These are called High-Level instructions.
So, in contrast with what was being stated about classic instructions that translate to one binary output 'word', the High-Level classic instructions can be translated into more than one binary output words.

As an example, take the `BEQ` instruction:

```
BEQ R<rs>,R<rt>, addr
```

This will be internally converted by the assembler to two seperate Low-Level instructions:

```
CMP R<rs>,R<rt>
BR EQ,addr
```

These two Low-Level instructions will result in two separate binary output words.
