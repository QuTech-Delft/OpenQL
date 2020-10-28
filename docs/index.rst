Welcome to OpenQL's documentation!
==================================

OpenQL is a framework for high-level quantum programming in C++/Python.
The framework provides a compiler for compiling and optimizing quantum code.
The compiler produces quantum assembly and instruction-level code for various target platforms.
While the instruction-level code is platform-specific, the quantum assembly code (QASM) is hardware-agnostic and can be simulated on one of the simulators.


.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   overview
   installation
   start


.. toctree::
   :maxdepth: 2
   :caption: OpenQL Basics

   program
   kernel
   quantum_gate
   classical_instructions
   platform
   compiler
   compiler_passes


.. toctree::
   :maxdepth: 1
   :caption: Colophon

   changelog
   contributors


.. toctree::
   :maxdepth: 1
   :caption: API Reference

   api/openql
   api/Kernel
   api/Program
   api/Compiler
   api/Platform
   api/Operation
   api/CReg

.. toctree::
   :maxdepth: 3
   :caption: Tutorials:

   qx_example
   dqcsim_example

.. toctree::
   :maxdepth: 1
   :caption: Developer Documentation

   cppref


Indices and tables

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
