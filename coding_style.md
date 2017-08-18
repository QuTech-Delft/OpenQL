OpenQL C++ Coding Conventions
=============================

In order to maintain the code homogeneous and consistent, all contibutors are invited to follow this coding convetion.

## Files Naming ##

C++ source files should be named `.cc` 
C++ header files should be named `.h`

Files should be named all lower case and separated by `_` when composed of multiple words: 
quantum_kernel, quantum_program, configuration_loader ...

## Variable/Function Naming ##

Use descriptive names, and be consistent in the style. Variable and class names should be all lower case and separated by `_` when composed of multiple words. 

## Custom Types Definition ##

When defining custom types using `typedef` , the type name should end with `_t` to distinguish it from instance and variable names.

## Getter/Setter ##

In the early developement phases of QpenQL, many attributes has been made public to speed up the developement and avoid writing getters/setters for the class attributes, obviously this not the proper way to code safely. From now on, add systematically getters/setters when defining private attributes and control setting with checks when needed.


