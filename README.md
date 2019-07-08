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

## Notes for Windows Users

* Use Power Shell for installation
* Set execution policy by:

        Set-ExecutionPolicy -ExecutionPolicy RemoteSigned

* Install [PowerShell Community Extensions](https://www.google.com "PowerShell Community Extensions")
* MSVC 2015 should be added to the path by using the following command:

        Invoke-BatchFile "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
* To make your life easier, you can add this command to the profile you are using for power shell, avoiding the need to manually run this command every time you open a power shell. You can see the path of profile by `echo $PROFILE`. Create/Edit this fille to add the above command.

* Python.exe and swig.exe should be in the path of power shell. To test if swig.exe is the path, run:

        Get-Command swig

* Make sure the following variables are defined:
  * PYTHON\_INCLUDE (should point to the directory containing Python.h)
  * PYTHON\_LIB (should point to the python library python35.lib)

# Installing OpenQL as a Conda Package

## use pre-built packages

Openql can be installed as a conda package (currently on Linux and Windows) by:

```sh
conda install -c imran.ashraf openql 
```

## use recipe to build package locally

conda packages can also be built locally by using the recipie available in `conda-recipe` directory, by running the following command:

```sh
conda build conda-recipe/.
```

The generated package can then be installed by:

```sh
conda install openql --use-local
```


# Installing OpenQL as Python Package

N.B. the instructions below will compile the C++ files the first time you try to install OpenQL package. If you are updating an existing installation you should first clean and recompile the C++ files using the following command. 

```sh
python cleanme.py
```

## Linux, Windows & OSX

Running the following command in Terminal/Power Shell should install the openql package:

```sh
python setup.py install  --user
```

Or

```sh
pip install  -e .
```

By defining NPROCS=N environment variable, multiple processos can be created for faster compilation. For example, the following command will create 4 processes for compilation:

```sh
NPROCS=4 python setup.py install  --user
```


## Running the tests

In order to pass all the tests, `qisa-as` and `libqasm` should be installed first. Follow [qisa-as](https://github.com/QE-Lab/eQASM_Assembler) and [libqasm](https://github.com/QE-Lab/libqasm) instructions to install python interfaces of these modules. Once `qisa-as` and `libqasm` are installed, you can run all the tests by:

```sh
py.test -v
```

Or

```sh
python -m pytest
```


# Compiling C++ OpenQL tests and examples

Existing tests and examples can be compiled by the following instructions. You can use an existing example as a starting point and write your own programs. Make sure to include them in CMakeLists.txt file to inform cmake to compile it as well.


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

<<<<<<< HEAD
Example C++ tests and examples can be found in 'tests' and 'examples'
directories. Executables for these will be generated in 'build/tests' and 'build/examples'
directory.

Example python tests and examples can be found in the 'tests' and 'examples' directories.
=======
Example C++ tests and programs can be found in 'tests' and 'examples'
directories. Executables for these will be generated in 'build/tests' and 'build/examples'
directory.

Example python tests and programs can be found in the 'tests' and 'examples' directories.
>>>>>>> develop
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
