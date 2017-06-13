# OpenQL Framework #

OpenQL is a C++ framework for high-level quantum programming. The framework
provide a compiler for compiling and optimizing quantum code. The compiler
produce the intermediate quantum assembly language and the compiled micro-code
for various target platforms. While the microcode is platform-specific, the
quantum assembly code (qasm) is hardware-agnostic and can be simulated on the
QX simulator.


## Supported Patforms

* Linux
* Windows
* OSX

## Required Packages

* g++ compiler with C++11 support (Linux)
* MSVC 2015 with update 3 (Windows)
* cmake
* swig
* python 3.5
* [Optional] Graphviz Dot utility to convert graphs from dot to pdf, png etc
* [Optional] XDot to visualize generated graphs in dot format


# Installing OpenQL as Python Package

## Linux & OSX

Running the following command in terminal should install the openql package:

        `python setup.py install --user`
## Windows

Python.exe and swig.exe should be in the path of power shell.

Make sure the following variables are defined:

* PYTHON\_INCLUDE (should point to the directory containing Python.h)
* PYTHON\_LIB (should point to the python library python35.lib)

Then running the following command in power shell should install the openql package:

        `python setup.py install`

## Verifying the installation
Run the following commands to test if the installation was successful 

        `tests/simplePyTest.py`
        `python setup.py tests/qubitsTest.py`    

# Compiling C++ OpenQL tests and programs

Existing tests and programs can be compiled by the following instructions. You can
use an existing example as a starting point and write your own programs. Make sure
to include them in CMakeLists.txt file to inform cmake to compile it as well.

## Linux

        mkdir cbuild
        cd cbuild
        cmake ..
        make

## Windows

        mkdir cbuild
        cd cbuild
        cmake -G "NMake Makefiles" ..
        nmake


## Usage

Example C++ tests and programs can be found in 'tests' and 'programs'
directories. Executables for these will be generated in 'build/tests' and 'build/programs'
directory.

Example python tests and programs can be found in 'tests' and 'programs' directories.
These can be executed as 'python tests/simplePyTest.py'.

