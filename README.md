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

For detailed user and contributor documentation, please visit the
[ReadTheDocs](https://openql.readthedocs.io) page.

## Dependencies

The following utilities are required to compile OpenQL from sources:

- C++ compiler with C++23 support (gcc 11, clang 14, msvc 17)
- `CMake` >= 3.12
- `git`
- `Python` 3.x plus `pip`, with the following package:
  - `conan` >= 2.0
 
### Python build specific dependencies

- `SWIG` (Linux: >= 3.0.12, Windows: >= 4.0.0)
- Optionally:
  - Documentation generation: `doxygen`
  - Convert graphs from `dot` to `pdf`, `png`, etc: `Graphviz Dot` utility
  - Visualize generated graphs in `dot` format: `XDot`
  - Use the visualizer in MacOS: `XQuartz`
- And the following Python packages:
  - `plumbum`
  - `qxelarator`
  - `setuptools`
  - `wheel`
  - Optionally:
    - Testing: `numpy`, and `pytest`
    - Documentation generation: `m2r2`, `sphinx==7.0.0`, and `sphinx-rtd-theme`

### ARM specific dependencies

We are having problems when using the `m4` and `zulu-opendjk` Conan packages on an ARMv8 architecture.
`m4` is required by Flex/Bison and `zulu-openjdk` provides the Java JRE required by the ANTLR generator.
So, for the time being, we are installing Flex/Bison and Java manually for this platform.

- `Flex` >= 2.6.4
- `Bison` >= 3.0
- `Java JRE` >= 11

## Build

This version of OpenQL can only be compiled via the `conan` package manager.
You'll need to create a default profile before using it for the first time.

The installation of `OpenQL` dependencies, as well as the compilation, can be done in one go.

```
git clone https://github.com/QuTech-Delft/OpenQL.git
cd OpenQL
conan profile detect
conan build . -pr=conan/profiles/tests-debug -b missing
```

Notice:
- the `conan profile` command only has to be run only once, and not before every build.
- the `conan build` command is building `OpenQL` in Debug mode with tests using the `tests-debug` profile.
- the `-b missing` parameter asks `conan` to build packages from sources
  in case it cannot find the binary packages for the current configuration (platform, OS, compiler, build type...).

### Build profiles

A group of predefined profiles is provided under the `conan/profiles` folder.
They follow the `[tests-](debug|release)[-unitary]` naming convention. For example:
- `release` is a Release build without tests and unitary decomposition disabled.
- `tests-debug-unitary` is a Debug build with tests and unitary decomposition enabled.

All the profiles set the C++ standard to 23.

### Build options

Profiles are a shorthand for command line options. The command above could be written as well as:

```
conan build . -s:h compiler.cppstd=23 -s:h openql/*:build_type=Debug -o openql/*:build_tests=True -o openql/*:disable_unitary=True -b missing
```

These are the list of options that could be specified whether in a profile or in the command line:

- `openql/*:build_type`: defaulted to `Release`, set to `Debug` if you want Debug builds.
- `openql/*:build_tests`: defaulted to `False`, set to `True` if you want to build tests.
- `openql/*:disable_unitary`: defaulted to `False`, set to `True` if you want to disable unitary decomposition.
- `openql/*:shared`: defaulted to `False`, set to `True` if you want OpenQL to be built as a shared library.
The default option is mandatory on Windows.

## Install

### From Python

Install from the project root directory as follows:

```
python3 -m pip install -v .
```

You can test if it works by running:

```
python3 -m pytest -v
```

### From C++

The `CMakeLists.txt` file in the root directory includes install targets:

```
conan create --version 0.11.2 . tests-debug -b missing
```

You can test if it works by doing:

```
cd test/Debug
ctest -C Debug --output-on-failure
```

## Use from another project

### From Python

After installation, you should be able to use the bindings for the original API by just `import openql as ql`.
The new API doesn't have Python bindings yet.

### From C++

The easiest way to use OpenQL in a CMake project is to fetch the library and then link against it.

```
include(FetchContent)
FetchContent_Declare(OpenQL
    GIT_REPOSITORY https://github.com/QuTech-Delft/OpenQL.git
    GIT_TAG "<a given cqasm git tag>"
)
FetchContent_MakeAvailable(OpenQL)
target_include_directories(<your target> SYSTEM PRIVATE "${OpenQL_SOURCE_DIR}/include")
target_link_libraries(<your target> PUBLIC ql)
```

Note that the following dependencies are required for `OpenQL` to build:

- `Flex` >= 2.6.4
- `Bison` >= 3.0
- `Java JRE` >= 11
 