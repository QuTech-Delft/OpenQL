.. _dev_build:

Build instructions
==================

This page documents how OpenQL and its documentation pages can be built and installed from scratch.

.. note::
   It is very difficult to maintain these instructions, due to there being so many supported environments,
   and due to externally-maintained dependencies. Therefore, please
   `let the OpenQL maintainers know <https://github.com/QuTech-Delft/OpenQL/issues/new>`_ if you run into any
   difficulties with these instructions. If you're a new maintainer, update them accordingly via a PR, but
   be mindful that something that works on your machine might not work on everyone's machine!

Dependencies
------------

The following packages are required to compile OpenQL from sources:

- C++ compiler with C++23 support (Linux: gcc, MacOS: LLVM/clang, Windows: Visual Studio 17 2022, MSVC 19.35.32217.1)
- CMake >= 3.12
- conan 2.0
- git
- Python 3.x + pip, with the following packages:

  - ``plumbum``
  - ``qxelarator``
  - ``setuptools``
  - ``wheel``
  - And, optionally, these:

    - Testing: ``libqasm``, ``make``, ``numpy``, and ``pytest``.
    - Documentation generation: ``doxygen``, ``m2r2``, ``sphinx==7.0.0``, and ``sphinx-rtd-theme``.
    - Convert graphs from `dot` to `pdf`, `png`, etc: ``Graphviz Dot utility``.
    - Visualize generated graphs in `dot` format: ``XDot``.
    - Use the visualizer in MacOS: ``XQuartz``.

- SWIG (Linux: >= 3.0.12, Windows: >= 4.0.0)

.. note::
   The connection between Sphinx and SWIG's autodoc functionalities is very iffy,
   but aside from tracking everything manually or forking SWIG there is not much that can be done about it.
   Because of this, not all Sphinx versions will build correctly,
   hence why the Sphinx version is pinned.
   Sphinx 4.x for example crashes on getting the function   signature of property getters/setters.

Windows-specific instructions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::
   The current maintainers of OpenQL all use either Linux or MacOS.
   While we've checked that these instructions should work on a clean Windows install, things may go out of date.
   Please let us know if you encounter difficulties with these instructions.

Dependencies can be installed with:

- `cmake 3.12.0 <https://github.com/Kitware/CMake/releases/download/v3.12.0/cmake-3.12.0-windows-x86_64.msi>`_
- `swigwin 4.0.0 <https://sourceforge.net/projects/swig/files/swigwin/swigwin-4.0.0/swigwin-4.0.0.zip/download>`_

Make sure the above mentioned binaries are added to the system path.

Alternatively, you can use Chocolatey to install packages.

The actual build and install should be done with PowerShell,
for which some modifications (may?) need to be made first.

- Use Power Shell for installation.
- Set execution policy by:

::

    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned

- Install PowerShell Community Extensions:

::

    Install-Module -AllowClobber -Name Pscx -RequiredVersion 3.2.2

- Visual Studio 17 2022 should be added to the path by using the following command:

::

    Invoke-BatchFile "C:\Program Files\Microsoft Visual Studio 17.0\VC\vcvarsall.bat" amd64

- but when you installed Microsoft Visual Studio Community Edition do:

::

    Invoke-BatchFile "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

- To make your life easier, you can add this command to the profile you are using for power shell,
  avoiding the need to manually run this command every time you open a power shell.
  You can see the path of this profile by `echo $PROFILE`. Create/Edit this file to add the above command.

- `Python.exe` and `swig.exe` should be in the path of power shell. To test if swig.exe is the path, run:

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
    brew install cmake doxygen graphviz llvm python3 swig xquartz
    pip3 install conan m2r2 numpy plumbum pytest setuptools qxelarator sphinx==3.5.4 sphinx-rtd-theme wheel

Make sure the above mentioned binaries are added to the system path in front of ``/usr/bin``,
otherwise CMake finds the default versions.

Linux-specific instructions
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Honestly, if you're already used to developing on Linux, and you're using a self-respecting Linux
distribution, you should have no problems installing these dependencies. None of them are particularly
special, so they should all be available in your package manager.

If you're for some reason using CentOS, you'll need to use a ``devtoolset`` compiler, because the one
shipped with it is too old. Likewise, CentOS ships with cmake 2.9 installed in ``/usr/bin`` and depends
on this; while ``cmake3`` is in the package manager, you actually need to call ``cmake3`` instead of
``cmake``, which ``setup.py`` is not smart enough for. On CentOS or other batteries-not-included systems
you might also have to compile some dependencies manually (``swig`` and possibly ``gettext``),
but they shouldn't give you too much drama. ``cmake`` has distro-agnostic binary distributions on github that are
only a ``wget`` and ``tar xzv`` away.


Obtaining OpenQL
----------------

OpenQL sources for each release can be downloaded from github
`releases <https://github.com/QuTech-Delft/OpenQL/releases>`_ as .zip or .tar.gz archive. OpenQL can also be
cloned by:

::

    git clone https://github.com/QuTech-Delft/OpenQL.git


Building the ``qutechopenql`` Python package
--------------------------------------------

Running the following command in a terminal/Power Shell from the root of the OpenQL repository should install the
``qutechopenql`` package:

::

    pip install -v .

Or in editable mode by the command:

::

    pip install -v -e .

Editable mode has the advantage that you'll get incremental compilation if you ever change OpenQL's C++ files,
but it's a bit more fragile in that things will break if you move the OpenQL repository around later.
Specifically, editable mode just installs an absolute path link to your clone of the OpenQL repository,
so if you move it, the link breaks.
You'd have to remember to uninstall if you ever end up moving it.

.. note::
   Depending on your system configuration,
   you may need to use ``pip3``, ``python -m pip`` or ``python3 -m pip`` instead of ``pip``.
   You may also need to add ``--user`` to the flags or prefix ``sudo``.
   An exhaustive list of which is needed when is out of scope here;
   instead, just look for pip usage instructions for your particular operating system online.
   This works the same for any other Python package.

.. warning::
   NEVER install with ``python3 setup.py install`` (or similar) directly!
   This always leads to all kinds of confusion,
   because ``setuptools`` does not inform ``pip`` that the package is installed, allowing ``pip`` to go out of sync.

.. note::
   The ``setup.py`` script (as invoked by pip in the above commands, again, do not invoke it directly!)
   listens to a number of environment variables to configure the installation and the compilation process:

   - ``OPENQL_BUILD_TYPE``: it can be ``Debug`` or ``Release``.
   - ``OPENQL_BUILD_TESTS``: defaulted to ``OFF``, set to ``ON`` if you want to build tests.
   - ``OPENQL_DISABLE_UNITARY``: defaulted to ``OFF``, set to ``ON`` if you want to disable unitary decomposition.
     This speeds up compile time if you don't need it.

   In bash-like terminals, you can just put them in front of the pip command like so:
   ``OPENQL_BUILD_TESTS=ON pip ...``.
   In Powershell, you can use ``$env:OPENQL_BUILD_TESTS = 'ON'`` in a command preceding the ``pip`` command.

.. note::
   You may find that CMake notes that some packages it's looking for are missing.
   This is fine: some things are only needed for optional components
   (which will automatically disable themselves when dependencies are missing) and
   some things are only quality-of-life things, for example for generating backtraces for the exception messages.
   As long as the tests pass, the core OpenQL components should all work.

Once installed, and assuming you have the requisite optional dependencies installed, you can run the test suite (still
from the root of the OpenQL repository) using

::

    pytest -v

.. note::
   If ``pytest`` is unrecognized, you should be able to use ``python -m pytest`` or ``python3 -m pytest`` instead
   (making sure to use the same Python version that the ``pip`` you installed the package with corresponds to).


Building the C++ tests and programs
-----------------------------------

Existing tests and programs can be compiled by the following instructions.
You can use any existing example as a starting point for your own programs.

The tests are run with the ``build/<build_type>`` directory as the working directory, so they can find their JSON files.
The results end up in a ``test_output`` folder, at the same location from where the tests are run
(``example_output`` if we are running an example instead of a test).


::

    conan build . -s:h compiler.cppstd=23 -s:h openql/*:build_type=Debug -o openql/*:build_tests=True -o openql/*:disable_unitary=True -b missing
    cd build/Debug
    ctest -C Debug --output-on-failure

.. note::

    The default option ``-o openql/*shared=False`` is mandatory on Windows
    because the executables can't find the OpenQL DLL in the build tree that MSVC generates,
    and static linking works around that.
    It works just fine when you manually place the DLL in the same directory as the test executables though,
    so this is just a limitation of the current build system for the tests.


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
   The Doxygen pages are never automatically rebuilt, as there is no dependency analysis here.
   You will always need to remove the doxygen output directory manually
   before calling ``make html`` to trigger a rebuild.
