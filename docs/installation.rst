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


