.. _dev_build:

Build instructions
==================

This page documents how OpenQL and its documentation pages can be built and installed from scratch.

.. note::
   It is very difficult to maintain these instructions, due to there being so many supported environments,
   and due to externally-maintained dependencies. Therefore, please
   `let the OpenQL maintainers know <https://github.com/QE-Lab/OpenQL/issues/new>`_ if you run into any
   difficulties with these instructions. If you're a new maintainer, update them accordingly via a PR, but
   be mindful that something that works on your machine might not work on everyone's machine!

Dependencies
------------

The following packages are required to compile OpenQL from sources:

- a C++ compiler with C++11 support (Linux: gcc, MacOS: LLVM/clang, Windows: MSVC 2015 with update 3 or above)
- git
- flex > 2.6
- bison > 3.0
- cmake >= 3.0
- swig (Linux: >= 3.0.12, Windows: >= 4.0.0)
- Python 3.x + pip, with the following packages:
   - ``plumbum``
   - ``wheel``
   - [Optional] ``pytest`` (for testing)
   - [Optional] ``numpy`` (for testing)
   - [Optional] ``libqasm`` (for testing)
   - [Optional] ``sphinx==3.5.4`` (for documentation generation)
   - [Optional] ``sphinx-rtd-theme`` (for documentation generation)
   - [Optional] ``m2r2`` (for documentation generation)
- [Optional] Doxygen (for documentation generation)
- [Optional] Graphviz Dot utility (to convert graphs from dot to pdf, png etc)
- [Optional] XDot (to visualize generated graphs in dot format)
- [Optional] GLPK (if you want initial placement support)
- [Optional] make (required for documentation generation; other CMake backends can be used for everything else)
- [Optional, MacOS only] XQuartz (only if you want to use the visualizer)

.. note::
   The connection between Sphinx' and SWIG's autodoc functionalities is very iffy, but aside from tracking everything
   manually or forking SWIG there is not much that can be done about it. Because of this, not all Sphinx versions will
   build correctly, hence why the Sphinx version is pinned. Sphinx 4.x for example crashes on getting the function
   signature of property getters/setters.

Windows-specific instructions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::
   The current maintainers of OpenQL all use either Linux or MacOS. While we've checked that these instructions
   should work on a clean Windows install, things may go out of date. Please let us know if you encounter
   difficulties with these instructions.

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

Alternatively, you can use Chocolatey to install packages. This is how CI currently does it. They just chain to
sourceforge downloads, though.

The actual build and install should be done with PowerShell, for which some modifications (may?) need to be made
first.

- Use Power Shell for installation
- Set execution policy by:

::

    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned

- Install PowerShell Community Extensions:

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

MacOS-specific instructions
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::
   These instructions have not been carefully vetted. If you run into issues, please let the maintainers know.

All dependencies can be installed using `Homebrew <https://brew.sh>`_ and pip:

::

    brew update
    brew install llvm flex bison cmake swig python3 doxygen graphviz glpk xquartz
    pip3 install wheel plumbum pytest numpy sphinx==3.5.4 sphinx-rtd-theme m2r2

Make sure the above mentioned binaries are added to the system path in front of ``/usr/bin``, otherwise CMake finds the default versions.

Linux-specific instructions
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Honestly, if you're already used to developing on Linux, and you're using a self-respecting Linux
distribution, you should have no problems installing these dependencies. None of them are particularly
special, so they should all be available in your package manager.

If you're for some reason using CentOS, you'll need to use a ``devtoolset`` compiler, because the one
shipped with it is too old. Likewise, CentOS ships with cmake 2.9 installed in ``/usr/bin`` and depends
on this; while ``cmake3`` is in the package manager, you actually need to call ``cmake3`` instead of
``cmake``, which ``setup.py`` is not smart enough for. On CentOS or other batteries-not-included systems
you might also have to compile some dependencies manually (``swig``, ``flex``, ``bison``, and their
dependencies ``m4`` and possibly ``gettext``), but they shouldn't give you too much drama. ``cmake`` has
distro-agnostic binary distributions on github that are a only ``wget`` and ``tar xzv`` away. ``glpk``
might be a bigger issue; I haven't tried.


Obtaining OpenQL
----------------

OpenQL sources for each release can be downloaded from github
`releases <https://github.com/QE-Lab/OpenQL/releases>`_ as .zip or .tar.gz archive. OpenQL can also be
cloned by:

::

    git clone https://github.com/QE-Lab/OpenQL.git --recursive

Note the ``--recursive``: the repository depends on various submodules. If you forgot the ``--recursive``,
you can get/synchronize them later with ``git submodule update --init --recursive``.


Building the ``qutechopenql`` Python package
--------------------------------------------

Running the following command in a terminal/Power Shell from the root of the OpenQL repository should install the
``qutechopenql`` package:

::

    pip install -v .

Or in editable mode by the command:

::

    pip install -v -e .

Editable mode has the advantage that you'll get incremental compilation if you ever change OpenQL's C++ files, but it's
a bit more fragile in that things will break if you move the OpenQL repository around later. Specifically, editable mode
just installs an absolute path link to your clone of the OpenQL repository, so if you move it, the link breaks. You'd have
to remember to uninstall if you ever end up moving it.

.. note::
   Depending on your system configuration, you may need to use ``pip3``, ``python -m pip`` or ``python3 -m pip`` instead
   of ``pip``. You may also need to add ``--user`` to the flags or prefix ``sudo``. An exhaustive list of which is needed
   when is out of scope here; instead, just look for pip usage instructions for your particular operating system online.
   This works the same for any other Python package.

.. warning::
   NEVER install with ``python3 setup.py install`` (or similar) directly! This always leads to all kinds of confusion,
   because ``setuptools`` does not inform ``pip`` that the package is installed, allowing ``pip`` to go out of sync.

.. note::
   The ``setup.py`` script (as invoked by pip in the above commands, again, do not invoke it directly!) listens to a number
   of environment variables to configure the installation and the compilation process. The most important ones are:

   - ``OPENQL_ENABLE_INITIAL_PLACEMENT``: if defined (value doesn't metter), initial placement support will be enabled.
   - ``OPENQL_DISABLE_UNITARY``: if defined (value doesn't matter), unitary decomposition is disabled. This speeds up
     compile time if you don't need it.
   - ``NPROCS``: sets the number of parallel processes to use when compiling (must be a number if defined). Without
     this, it won't multithread, so it'll be much slower.

   In bash-like terminals, you can just put them in front of the pip command like so: ``NPROCS=10 pip ...``. In
   Powershell, you can use ``$env:NPROCS = '10'`` in a command preceding the ``pip`` command.

.. note::
   You may find that CMake notes that some packages it's looking for are missing. This is fine: some things are only
   needed for optional components (which will automatically disable themselves when dependencies are missing) and
   some things are only quality-of-life things, for example for generating backtraces for the exception messages.
   As long as the tests pass, the core OpenQL components should all work.

Once installed, and assuming you have the requisite optional dependencies installed, you can run the test suite (still
from the root of the OpenQL repository) using

::
    pytest -v

.. note::
   If ``pytest`` is unrecognized, you should be able to use ``python -m pytest`` or ``python3 -m pytest`` instead
   (making sure to use the same Python version that the ``pip`` you installed the package with corresponds to).

Conda vs pip
^^^^^^^^^^^^

A conda recipe also exists in the repository. However, it is in a state of disuse, as conda's ridiculous NP-complete
dependency solver implementation is too heavy for CI (it can take literal hours), and none of the maintainers use it.
Your mileage may vary.


Building the C++ tests and programs
-----------------------------------

Existing tests and programs can be compiled by the following instructions. You
can use any existing example as a starting point for your own programs, but
refer to ``examples/cpp-standalone-example`` for the build system.

The tests are run with the ``tests`` directory as the working directory, so
they can find their JSON files. The results end up in ``tests/test_output``.


Linux/MacOS
^^^^^^^^^^^

Existing tests and examples can be compiled and run using the following commands:

::

    mkdir cbuild
    cd cbuild
    cmake .. -DOPENQL_BUILD_TESTS=ON    # configure the build
    make                                # actually build OpenQL and the tests
    make test                           # run the tests


Windows
^^^^^^^

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
^^^^^^^^^^^^^^^^^

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


Building the documentation
--------------------------

If you want, you can build the ReadTheDocs and doxygen documentation locally for your particular version of OpenQL.
Assuming you have installed the required dependencies to do so, the procedure is as follows.

::

    # first build/install the qutechopenql Python package!
    cd docs
    rm -rf doxygen      # optional: ensures all doxygen pages are rebuilt
    make clean          # optional: ensures all Sphinx pages are rebuilt
    make html

The main page for the documentation will be generated at ``docs/_build/html/index.html``.

.. note::
   The Doxygen pages are never automatically rebuilt, as there is no dependency analysis here. You will always need
   to remove the doxygen output directory manually before calling ``make html`` to trigger a rebuild.
