# OpenQL Framework #

[![Documentation Status](https://readthedocs.org/projects/openql/badge/?version=latest)](https://openql.readthedocs.io/en/latest/?badge=latest)
[![Travis Build Status](https://travis-ci.com/QE-Lab/OpenQL.svg?branch=develop)](https://travis-ci.com/QE-Lab/OpenQL)
[![GitHub Actions Build Status](https://github.com/QE-Lab/OpenQL/workflows/Test/badge.svg)](https://github.com/qe-lab/OpenQL/actions)
[![PyPI](https://badgen.net/pypi/v/qutechopenql)](https://pypi.org/project/qutechopenql/)
[![Anaconda](https://anaconda.org/qe-lab/openql/badges/version.svg)](https://anaconda.org/qe-lab/openql/)

OpenQL is a framework for high-level quantum programming in C++/Python. The framework provides a compiler for compiling and optimizing quantum code. The compiler produces the intermediate quantum assembly language and the compiled micro-code for various target platforms. While the microcode is platform-specific, the quantum assembly code (qasm) is hardware-agnostic and can be simulated on the QX simulator. For detailed documentation see [here](https://openql.readthedocs.io).

## Supported Patforms

* Linux
* Windows
* OSX

## Installation

OpenQL can be installed in a number of ways, See [Installation](https://openql.readthedocs.io/en/latest/installation.html) for details.

## Usage

Example python tests and programs can be found in the 'tests' and 'examples' directories. These can be executed as 'python tests/simplePyTest.py'.

Example C++ tests and programs can be found in 'tests' and 'examples'
directories. Executables for these will be generated in 'cbuild/tests' and 'cbuild/examples' directory. An executable can be executed as: './example'



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
