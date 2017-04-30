# OpenQL Framework #

OpenQL is a C++ framework for high-level quantum programming. The framework
provide a compiler for compiling and optimizing quantum code. The compiler
produce the intermediate quantum assembly language and the compiled micro-code
for various target platforms. While the microcode is platform-specific, the
quantum assembly code (qasm) is hardware-agnostic and can be simulated on the
QX simulator.


## Supported Patforms

* Linux
* Windows (coming soon)

## Required Packages

* g++ compiler with C++ 11 support (Linux)
* MSVC 2015 (Windows)
* cmake
* swig
* [Optional] XDot to visualize generated graphs in dot format
* [Optional] Graphviz Dot utility to convert graphs from dot to pdf, png etc

## Setup

# Linux

        mkdir build
        cd build
        cmake ..
        make

# Windows

        mkdir build
        cd build
        cmake -G "NMake Makefiles" ..
        nmake

## Installing Python Package

# Linux

Running the following command in terminal should intall openql package:

        python setup.py install --user


# Windows

Python.exe and swig.exe should be in the path.

Make sure the following variables are defined:

* PYTHON\_INCLUDE (should point to the directory containing Python.h, for instance, on my pc it is C:\Users\iashraf\AppData\Local\Programs\Python\Python35\include)

* PYTHON\_LIB (should point to the python library, for instance, on my pc it is C:\Users\iashraf\AppData\Local\Programs\Python\Python35\libs\python35.lib)

Then running the following command in power shell should intall openql package:

        python setup.py install



## Usage

Executables will be generated in build/tests and build/programs directory.
Further details coming soon ...

