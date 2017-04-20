# OpenQL Framework #

OpenQL is a C++ framework for high-level quantum programming. The framework
provide a compiler for compiling and optimizing quantum code. The compiler
produce the intermediate quantum assembly language and the compiled micro-code
for various target platforms. While the microcode is platform-specific, the
quantum assembly code (qasm) is hardware-agnostic and can be simulated on the
QX simulator.


## Supported Patforms

* Linux
* Windows (coming soon)

## Required Packages

* g++ compiler with C++ 11 support
* cmake
* [Lemon graph library](http://lemon.cs.elte.hu/trac/lemon)
* graphviz Dot utility to convert graphs from dot to pdf, png etc

## Setup

        mkdir build
        cd build
        cmake ..
        make


## Usage

coming soon
