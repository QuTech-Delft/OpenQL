# OpenQL Framework #

[![Documentation Status](https://readthedocs.org/projects/openql/badge/?version=latest)](https://openql.readthedocs.io/en/latest/?badge=latest)
[![Travis Build Status](https://travis-ci.com/QuTech-Delft/OpenQL.svg?branch=develop)](https://travis-ci.com/QuTech-Delft/OpenQL)
[![GitHub Actions Build Status](https://github.com/QuTech-Delft/OpenQL/workflows/Test/badge.svg)](https://github.com/qutech-delft/OpenQL/actions)
[![PyPI](https://badgen.net/pypi/v/qutechopenql)](https://pypi.org/project/qutechopenql/)
[![Anaconda](https://anaconda.org/qutech/openql/badges/version.svg)](https://anaconda.org/qutech/openql/)

OpenQL is a framework for high-level quantum programming in C++/Python. The
framework provides a compiler for compiling and optimizing quantum code. The
compiler produces the intermediate quantum assembly language and the compiled
micro-code for various target platforms. While the microcode is
platform-specific, the quantum assembly code
(in [cQASM](https://libqasm.readthedocs.io/) format) is hardware-agnostic and
can be simulated on the QX simulator.

OpenQL's source code is released under the Apache 2.0 license.

For detailed user and contributor documentation, visit the
[the ReadTheDocs](https://openql.readthedocs.io) page!
