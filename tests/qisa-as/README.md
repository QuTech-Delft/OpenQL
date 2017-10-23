### Assembler for the Quantum Instruction Set Architecture
---
```
Usage: qisa-as [OPTIONS] INPUT_FILE
Assembler/Disassembler for the Quantum Instuction Set Architecture (QISA).

Options:
  -d                Disassemble the given INPUT_FILE
  -o OUTPUT_FILE    Save binary assembled or textual disassembled instructions to the given OUTPUT_FILE
  -t                Enable scanner and parser tracing while assembling
  -V, --version     Show the program version and exit
  -v, --verbose     Show informational messages while assembling
  -h, --help        Show this help message and exit
```
---

The early versions were based on ideas from:
- https://github.com/jonathan-beard/simple_wc_example
- https://github.com/optixx/mycpu

#### Dependencies

In order to build this assembler, you need CMake, bison, flex, and a C++
compiler that supports C++11.

A python interface is also provided: This necessitates swig and the python
development environment.

##### Linux

The Linux distribution on which this software has been tested is Ubuntu 16.04.
The following software packages are needed:

| Package       | Description                                        |
| ------------- | -------------------------------------------------- |
| `bison`       | Context-free grammar parser generator              |
| `cmake`       | Cross-platform build tool                          |
| `flex`        | Lexical analyser generator                         |
| `g++-5`       | C++ compiler that supports C++11                   |
| `python3`     | Interpreter for the Python language, version 3.x   |
| `python3-dev` | Development headers and libraries for `python3`    |
| `swig`        | Tool used to call C++ functions from Python        |

These packages can be installed using the following command line:
`sudo apt-get install bison cmake flex g++-5 python3 python3-dev swig`


##### Windows

CMake can be installed from: https://cmake.org/download
You can use either the Windows win64-x64 or the win32-x86 installer, depending
on your Windows installation.

###### Bison & Flex
For Bison and flex, you can install the latest win\_flex\_bison package from
https://sourceforge.net/projects/winflexbison/files
It is distributed as a zip archive, which must be unpacked in a separate
directory.
Add that directory to your Windows PATH variable.

> __IMPORTANT__:
>
> Be sure to download the latest win_flex_bison __*2.5.x*__ version that includes Bison version 3.x!
>
> Do __NOT__ use the _2.4.x_ version!

The version that is known to work for this assembler is `2.5.10`.

###### C++

For C++, you can install Visual Studio 2017 Community Edition
(https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15#).

###### Python
Python can be retrieved from https://www.python.org/downloads/windows/

You need to get the latest 'Windows x86-64 executable installer'

If you want, there is a nice tutorial that might help if you need more
information:
https://www.howtogeek.com/197947/how-to-install-python-on-windows/

In order for this Python installation to be picked up by cmake, you need to
define two environment variables:

- PYTHON_LIBRARY             - path to the python library
- PYTHON_INCLUDE_DIR         - path to where Python.h is found

Example using Python 3.6.1:
```
PYTHON_INCLUDE: C:\Python36\include
PYTHON_LIB: C:\Python36\libs\python36.lib
```

###### SWIG

SWIG is used to provide the python interface to the QISA assembler.
It can be downloaded from:
https://sourceforge.net/projects/swig/files/swigwin/

Take the latest version.

SWIG for Windows is distributed as a zip archive, which must be unpacked in a
separate directory.
Add that directory to your Windows PATH variable.

#### Building

CMake is used to generate the platform specific Makefile/Solution file with
which the 'qisa-as' executable can be built.

##### Linux

Create a directory 'build' in this directory.
From within that directory, invoke CMake and then make.
In commands:

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

The 'qisa-as' executable can be found in this directory afterwards.

Built and tested on Ubuntu 16.04.

##### Windows (Using Visual Studio IDE)

* Create an out-of-source 'build' directory. (A directory outside of <CCLight root>/qisa-as).
* Start up 'CMake-gui'
* Using the 'Browse Source...' button, locate the directory <CCLight root>/qisa-as.
* Using the 'Browse Build...' button, locate the 'build' directory that has just
  been created.
* Now press on the 'Configure' button.
  There will be a dialog in which you can specify the kind of CMake generator to use.
  For the tested case, Visual Studio 15 2017 was used, along with the option to
  use default native compilers.
* Now press the 'Generate' button.

This should have generated a (in case of 'Visual Studio 15 2017') solution in
the chosen 'build' directory, named 'qisa-as.sln'.

* Open the generated 'qisa-as.sln' using Visual Studio 15 2017
* Select the 'Solution configuration' you want (By default, this is Debug, but
  you might want to select 'Release' instead).
* Press 'F7' to build the solution.

The 'qisa-as.exe' executable can be found in the 'build/Release' directory after
the compilation has terminated.
> Note that 'qisa-as.exe' depends on the shared library 'qisa-as-lib.dll', which must be installed in the same directory as 'qisa-as.exe'.

Built and tested on Windows 10.

##### Windows (Using Powershell)

* Create an out-of-source 'build' directory. (A directory outside of
  <CCLight root>/qisa-as).
* Start up Powershell and go to that directory.
* `cmake -G "NMake Makefiles" <CCLight root>/qisa-as`
* `nmake`

> Note: This is a debug build by default...

The 'qisa-as.exe' executable can be found in the 'build' directory after the
compilation has terminated.

> Note that 'qisa-as.exe' depends on the shared library 'qisa-as-lib.dll', which must be installed in the same directory as 'qisa-as.exe'.

Built and tested on Windows 10.

#### Testing

In directory 'qisa\_test\_assembly', you can find some assembly source files that
you can use to test the generated assembler.
The file 'test\_assembly.qisa' contains all known 'classic' instructions and aliases,
and some quantum instructions.


#### Python interface

The provided python interface is generated using SWIG.
Look in the file 'qisa-as-swig.i' to see the signature of the provided
functions.

In order to use the python interface from another location than the <build_dir>,
you will have to copy some files from there to the chosen installation
directory.

These are:
##### Linux
* libqisa-as-lib.so
* _pyQisaAs.so
* pyQisaAs.py

##### Windows
* qisa-as-lib.dll
* _pyQisaAs.pyd
* pyQisaAs.py

In the directory 'test\_python\_interface' you can find an example python script
that shows how to assemble a QISA assembly file, and disassemble the
generated binary assembly file afterwards.

A README.md file describes how you might run it.

#### Implementation notes

Flexibility has been the focus of the current qisa-as implementation.
This is because the opcodes of the classic instructions are not yet final.
Also, the quantum instructions themselves are not fully specified and are subject to revisal.

The solution found is to have the specification of the opcodes of the classic instructions and that of the
quantum instructions (instruction and opcode) into a text file (`qisa_opcodes.qmap`).
This file is processed during build time, and generates the appropriate cpp, bison and flex input files.
The generated files are put in the build directory. They are: `qisa_parser.yy`, `qisa_lexer.l` and
`qisa_opcode_defs.inc`.
The input files needed to generate those files are:

* `qisa_opcodes.qmap` Contains the opcode definitions for the classic instructions and the quantum
instructions (declared using several formats). Refer to this file for more information.
* `qisa_parser.tmpl` Contains the grammars for the qisa-as assembler.
* `qisa_lexer.tmpl` Contains the syntax definition for the qisa-as assembler.


`qisa_opcodes.qmap` is processed by a Python script, which uses string templates.
One peculiarity of using Python's string template substitution is that `$$` stands for an escaped dollar sign.
The net result of this is that every occurrence of `$$` in the template file is converted to a single `$`.
Normally this is not a problem, but in the Bison parser language, this `$$` construct has to be used.
To be able to use the `$$` construct, the template file should contain `$$$$`.
Keep that in mind if modifying `qisa_parser.tmpl`.
