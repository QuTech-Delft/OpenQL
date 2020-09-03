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

.. note::

    Initial placement is currently not included with the binary distribution
    due to a license incompatibility with one of the dependencies (GLPK, to
    be specific). Until this is resolved somehow, you'll have to build from
    source if you need it.


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

    conda install -c qe-lab openql


Conda packages can also be built locally by using the recipe available in the conda-recipe directory,
by running the following command from the OpenQL directory:

::

    conda build conda-recipe/.

The generated package can then be installed by:

::

    conda install openql --use-local


Compilation from sources
------------------------

Compiling OpenQL from sources involves:

- Setting-up required packages
- Obtaining OpenQL


Required Packages
^^^^^^^^^^^^^^^^^

The following packages are required to compile OpenQL from sources:

- g++ compiler with C++11 support (Linux)
- MSVC 2015 with update 3 or above (Windows)
- flex (> 2.6)
- bison (> 3.0)
- cmake (>= 3.0)
- swig (Linux: 3.0.12, Windows: 4.0.0)
- Python (3.5, 3.6, 3.7)
- [Optional] pytest used for running tests
- [Optional] Graphviz Dot utility to convert graphs from dot to pdf, png etc
- [Optional] XDot to visualize generated graphs in dot format
- [Optional] GLPK if you want initial placement support


Notes for Windows Users
^^^^^^^^^^^^^^^^^^^^^^^
Dependencies can be installed with:

- `win_flex_bison 2.5.20 <https://sourceforge.net/projects/winflexbison/files/win_flex_bison-2.5.20.zip/download>`_
- `cmake 3.15.3 <https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-win64-x64.msi>`_
- `swigwin 4.0.0 <https://sourceforge.net/projects/swig/files/swigwin/swigwin-4.0.0/swigwin-4.0.0.zip/download>`_

Make sure the above mentioned binaries are added to the system path.

For initial placement support, you'll also need
`winglpk 4.6.5 <https://sourceforge.net/projects/winglpk/files/winglpk/GLPK-4.65/winglpk-4.65.zip/download>`_.
But just adding this directory to the system path is not enough for CMake to find it. Instead, the toplevel
CMake script listens to the ``WINGLPK_ROOT_DIR`` environment variable. Set that to the root directory of what's
in that zip file instead.


- Use Power Shell for installation
- Set execution policy by:

::

    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned

- Install [PowerShell Community Extensions] (https://www.google.com "PowerShell Community Extensions")

::

    Install-Module -AllowClobber -Name Pscx -RequiredVersion 3.2.2

- MSVC 2015 should be added to the path by using the following command:

::

    Invoke-BatchFile "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

- but when you installed Microsoft Visual Studio Community Edition do:

::

    Invoke-BatchFile "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

- To make your life easier, you can add this command to the profile you are using for power shell, avoiding the need to manually run this command every time you open a power shell. You can see the path of this profile by `echo $PROFILE`. Create/Edit this file to add the above command.

- Python.exe, win_flex.exe, win_bison.exe and swig.exe should be in the path of power shell. To test if swig.exe is the path, run:

::

    Get-Command swig

- To show the currently defined environment variables do:

::

    Gci env:

- Make sure the following variables are defined:

    - PYTHON_INCLUDE (should point to the directory containing Python.h)
    - PYTHON_LIB (should point to the python library pythonXX.lib, where XX is for the python version number)

- To set an environment variable in an expression use this syntax:

::

    $env:EnvVariableName = "new-value"

Obtaining OpenQL
^^^^^^^^^^^^^^^^

OpenQL sources for each release can be downloaded from github `releases <https://github.com/QE-Lab/OpenQL/releases>`_ as .zip or .tar.gz archive. OpenQL can also be cloned by:

::

    git clone https://github.com/QE-Lab/OpenQL.git --recursive


Compiling OpenQL as Python Package
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Running the following command in the python (virtual) environment in Terminal/Power Shell should install the openql package:

::

    cd OpenQL
    git submodule update --init --recursive
    pip install -v

Or in editable mode by the command:

::

    pip install -v -e .

Editable mode has the advantage that you'll get incremental compilation if you ever change OpenQL's C++ files, but it's
a bit more fragile in that things will break if you move the OpenQL repository around later. Specifically, editable mode
just installs an absolute path link to your clone of the OpenQL repository, so if you move it, the link breaks. You'd have
to remember to uninstall if you ever end up moving it.

.. note::

    The ``setup.py`` script (as invoked by pip in the above commands) listens to a number of environment variables to
    configure the installation and the compilation process. The most important ones are:

    - ``OPENQL_ENABLE_INITIAL_PLACEMENT``: if defined (value doesn't metter), initial placement support will be enabled.
    - ``OPENQL_DISABLE_UNITARY``: if defined (value doesn't matter), unitary decomposition is disabled. This speeds up
      compile time if you don't need it.
    - ``NPROCS``: sets the number of parallel processes to use when compiling (must be a number if defined). Without
      this, it won't multithread, so it'll be much slower.

    In bash-like terminals, you can just put them in front of the pip command like so: ``NPROCS=10 pip ...``. In
    Powershell, you can use ``$env:NPROCS = '10'`` in a command preceding the ``pip`` command.


Running the tests
.................

In order to pass all the python tests, the openql package should be installed in editable mode.
Also, *qisa-as* and *libqasm* should be installed first. Follow `qisa-as <https://github.com/QE-Lab/eQASM_Assembler>`_
and `libqasm <https://github.com/QE-Lab/libqasm>`_ instructions to install python interfaces of these modules.
Once *qisa-as* and *libqasm* are installed, you can run all the tests by:

::

    pytest -v


or

::

    python -m pytest


Compiling C++ OpenQL tests and programs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Existing tests and programs can be compiled by the following instructions. You
can use any existing example as a starting point for your own programs, but
refer to ``examples/cpp-standalone-example`` for the build system.

The tests are run with the ``tests`` directory as the working directory, so
they can find their JSON files. The results end up in ``tests/test_output``.


Linux/OSX
.........

Existing tests and examples can be compiled and run using the following commands:

::

    mkdir cbuild
    cd cbuild
    cmake .. -DOPENQL_BUILD_TESTS=ON    # configure the build
    make                                # actually build OpenQL and the tests
    make test                           # run the tests


Windows
.......

Existing tests and examples can be compiled and run using the following commands:

::

    mkdir cbuild
    cd cbuild
    cmake .. -DOPENQL_BUILD_TESTS=ON -DBUILD_SHARED_LIBS=OFF # configure the build
    cmake --build .                     # actually build OpenQL and the tests
    cmake --build . --target RUN_TESTS  # run the tests

.. note::

    ``-DBUILD_SHARED_LIBS=OFF`` is needed on Windows only because the
    executables can't find the OpenQL DLL in the build tree that MSVC
    generates, and static linking works around that. It works just fine when
    you manually place the DLL in the same directory as the test executables
    though, so this is just a limitation of the current build system for the
    tests.

Other CMake flags
.................

CMake accepts a number of flags in addition to the ``-DOPENQL_BUILD_TESTS=ON``
flag used above:

 - ``-DWITH_INITIAL_PLACEMENT=ON``: enables initial placement.
 - ``-DWITH_UNITARY_DECOMPOSITION=OFF``: disables unitary composition (vastly
   speeds up compile time if you don't need it).
 - ``-DCMAKE_BUILD_TYPE=Debug``: builds in debug rather than release mode
   (less optimizations, more debug symbols).
 - ``-DBUILD_SHARED_LIBS=OFF``: build static libraries rather than dynamic
   ones. Note that static libraries are not nearly as well tested, but they
   should work if you need them.
