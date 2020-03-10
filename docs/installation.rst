Installation
============

OpenQL is supported on Linux, Windows and OSX. OpenQL can be installed on
these platforms as a pre-built package as well as can be compiled from
sources.

- Pre-built package
	- python package using pip
	- conda package
- Compilation from sources
	- Windows
	- Linux
	- OSx


Installing the pre-built package
--------------------------------

Pre-built packages are available for OpenQL.



Pre-built Wheels
^^^^^^^^^^^^^^^^

This is perhaps the easiest way to get OpenQL running on your machine.

Pre-built OpenQL wheels are available for 64-bit Windows, Linux and OSX. These
wheels are available for Python 3.5, 3.6 and 3.7. OpenQL can be installed by
the command:

::

    pip install qutechopenql


.. note::

    ``python`` refers to Python 3 and ``pip`` refers to Pip 3, corresponding to Python version 3.5, 3.6 or 3.7.


Conda package
^^^^^^^^^^^^^

OpenQL can be installed as a conda package (currently on Linux and Windows only) by:

::

    conda install -c imran.ashraf openql


Conda packages can also be built locally by using the recipe available in the conda-recipe directory,
by running the following command from the OpenQL directory:

::

    conda build conda-recipe/.

The generated package can then be installed by:

::

    conda install openql --use-local


Compilation from sources
------------------------

Compiling OpenQL from sources involves

- Setting-up required packages
- Obtaining OpenQL


Required Packages
^^^^^^^^^^^^^^^^^

The following packages are required to compile OpenQL from sources:

- g++ compiler with C++11 support (Linux)
- MSVC 2015 with update 3 or above (Windows)
- cmake (>= 3.0)
- swig
- python (3.5, 3.6, 3.7)
- [Optional] pytest used for running tests
- [Optional] Graphviz Dot utility to convert graphs from dot to pdf, png etc
- [Optional] XDot to visualize generated graphs in dot format


Notes for Windows Users
^^^^^^^^^^^^^^^^^^^^^^^

- Use Power Shell for installation
- Set execution policy by:

::

    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned

- Install [PowerShell Community Extensions] (https://www.google.com "PowerShell Community Extensions")
- MSVC 2015 should be added to the path by using the following command:

::

    Invoke-BatchFile "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

- To make your life easier, you can add this command to the profile you are using for power shell, avoiding the need to manually run this command every time you open a power shell. You can see the path of this profile by `echo $PROFILE`. Create/Edit this file to add the above command.

- Python.exe and swig.exe should be in the path of power shell. To test if swig.exe is the path, run:

::

    Get-Command swig

- Make sure the following variables are defined:

    - PYTHON_INCLUDE (should point to the directory containing Python.h)
    - PYTHON_LIB (should point to the python library pythonXX.lib, where XX is for the python version number)



Obtaining OpenQL
^^^^^^^^^^^^^^^^

OpenQL sources for each release can be downloaded from github `releases <https://github.com/QE-Lab/OpenQL/releases>`_ as .zip or .tar.gz archive. OpenQL can also be cloned by:

::

    git clone https://github.com/QE-Lab/OpenQL.git


Compiling OpenQL as Python Package
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Running the following command in Terminal/Power Shell should install the openql package:

::

    python setup.py install  --user

Or in editable mode by the command:

::

    pip install  -e .[develop]


Compiling C++ OpenQL tests and programs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Existing tests and programs can be compiled by the following instructions. You can use an existing example as a starting point and write your own programs. Make sure to include them in the CMakeLists.txt file to inform cmake to compile it as well.


Linux/OSX
.........

Existing tests and programs can be compiled on Linux OS by the following commands:

::

    mkdir cbuild
    cd cbuild
    cmake ..   # generates the make file based on CMakeLists.txt in the OpenQL directory
    make       # compiles the source code into the current directory.



To execute the given examples/test, go to e.g., ```OpenQL/cbuild/examples``` and execute one of the files e.g.,  ```./simple```. The output will be saved to the output directory next to the file.

If one wants to compile and run a single file without adding it to CMakeLists.txt, e.g., ```example.cc```, he can use the standalone example provided in ```examples/cpp-standalone-example``` directory.



Windows
.......

::

    mkdir cbuild
    cd cbuild
    cmake -G "NMake Makefiles" ..
    nmake


Running the tests
-----------------

In order to pass all the python tests, the openql package should be installed in editable mode.
Also, *qisa-as* and *libqasm* should be installed first. Follow `qisa-as <https://github.com/QE-Lab/eQASM_Assembler>`_
and `libqasm <https://github.com/QE-Lab/libqasm>`_ instructions to install python interfaces of these modules.
Once *qisa-as* and *libqasm* are installed, you can run all the tests by:

::

    pytest -v


or

::

    python -m pytest



