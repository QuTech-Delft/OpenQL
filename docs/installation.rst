Installation
============

OpenQL is supported on Linux, Windows and OSX. OpenQL can be installed on these platforms as a pre-built package as well as can be compiled from sources.

- Pre-built package
	- python package using pip
	- conda package
- Compilation from sources
	- Windows
	- Linux
	- OSx


Installing Pre-built package
-----------------------------

Pre-built packges are avialable for openql.

Pre-built Wheels
^^^^^^^^^^^^^^^^

This is perhaps the most easiest way to get OpenQL running on your machine. Prebuilt OpenQL wheels are available for 64-bit Windows, Linux and OSX. These wheels are available for Python 3.5, 3.6 and 3.7. OpenQL can be installed by using one of these pre-built wheels for the specific python version and platform at hand, by the command:

::

    pip install <openql*.whl>

For example, on 64-bit linux,

::

    pip install https://github.com/QE-Lab/OpenQL/releases/download/0.8.0/openql-0.8.0-cp35-cp35m-linux_x86_64.whl


Conda package
^^^^^^^^^^^^^

Openql can be installed as a conda package (currently on Linux and Windows) by:

::

    conda install -c imran.ashraf openql 


conda packages can also be built locally by using the recipie available in conda-recipe directory, by running the following command from OpenQL directory:

::

    conda build conda-recipe/.

The generated package can then be installed by:

::

    conda install openql --use-local


Compilation from sources
------------------------

Compiling OpenQL from sources involves

- Setting up required packages
- Obtaining OpenQL


Required Packages
^^^^^^^^^^^^^^^^^

The following packges are required to compile OpenQL from sources:

- g++ compiler with C++11 support (Linux)
- MSVC 2015 with update 3 or above (Windows)
- cmake (>= 3.0)
- swig
- python (3.5, 3.6, 3.7)
- [Optional] pytest used for running tests
- [Optional] Graphviz Dot utility to convert graphs from dot to pdf, png etc
- [Optional] XDot to visualize generated graphs in dot format


.. note::

    In all the following instructions, python refers to Python 3 and pip refers to Pip 3.


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

    pip install  -e .

