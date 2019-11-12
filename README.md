# OpenQL Framework #

OpenQL is a framework for high-level quantum programming in C++/Python. The framework provides a compiler for compiling and optimizing quantum code. The compiler produces the intermediate quantum assembly language and the compiled micro-code for various target platforms. While the microcode is platform-specific, the quantum assembly code (qasm) is hardware-agnostic and can be simulated on the QX simulator.

## Supported Patforms

* Linux
* Windows
* OSX

## Required Packages

The following packges are required if you are not using pre-built conda packages:

* g++ compiler with C++11 support (Linux)
* MSVC 2015 with update 3 or above (Windows)
* cmake (>= 3.0)
* swig
* python (>= 3.5)
* [Optional] pytest used for running tests
* [Optional] Graphviz Dot utility to convert graphs from dot to pdf, png etc
* [Optional] XDot to visualize generated graphs in dot format

In all the instructions below, `python` refers to `Python 3` and `pip` refers to `Pip 3`.





# Compiling C++ OpenQL tests and programs

Existing tests and programs can be compiled by the following instructions. You can use an existing example as a starting point and write your own programs. Make sure to include them in CMakeLists.txt file to inform cmake to compile it as well.


## Linux/OSX

```sh
mkdir cbuild
cd cbuild 
cmake ..   # generates the make file based on CMakeLists.txt in the OpenQL directory
make       # compiles the source code into the current directory. 
```

To execute an example program go to e.g., `OpenQL/cbuild/examples` and execute one of the files e.g.,  `./simple`. The output will be saved to the output directory next to the file.

If one wants to compile and run a single file, e.g., `example.cc`, to compile it one can run : 

```sh
mkdir output           # create an output directory if it does not exist
g++ -std=c++11 example.cc -o example.exe -I OpenQL/   # compile the file
./example.exe                                         # execute the file
```

## Windows

```
mkdir cbuild
cd cbuild
cmake -G "NMake Makefiles" ..
nmake
```

## Usage

Example C++ tests and programs can be found in 'tests' and 'examples'
directories. Executables for these will be generated in 'build/tests' and 'build/examples'
directory.

Example python tests and programs can be found in the 'tests' and 'examples' directories.
These can be executed as 'python tests/simplePyTest.py'.

# Getting started 

After installing OpenQL a good place to get started is by looking at the files
in the "tests" directory. Here you can find commented examples on how to use OpenQL.
For instance, `examples/getting_started.py` could be a good starting point.
`doc` directory as well as the Wiki page documents various aspects.

N.B. gates in OpenQL are *case insensitive*. 

# Changelog

Please read [CHANGELOG.md](CHANGELOG.md) for more details.


# Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for more details.
Typically you will be need to perform the following steps to contribute:

1. `Fork` this repository
1. Create a `branch`
1. `Commit` your changes
1. `Push` your `commits` to the `branch`
1. Submit a `pull request`

You can find more information about Pull Requests [here](https://help.github.com/categories/collaborating-on-projects-using-pull-requests/)

# Contributors
Please see [CONTRIBUTORS.md](CONTRIBUTORS.md).
