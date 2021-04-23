Welcome to OpenQL's documentation!
==================================

OpenQL is a framework for high-level quantum programming in C++/Python.
The framework provides a compiler for compiling and optimizing quantum code.
The compiler produces quantum assembly and instruction-level code for various target platforms.
While the instruction-level code is platform-specific, the quantum assembly code (QASM) is hardware-agnostic and can be simulated on one of the simulators.


.. toctree::
   :maxdepth: 2
   :caption: Getting started

   overview
   installation
   start

.. toctree::
   :maxdepth: 2
   :caption: OpenQL basics

   program
   kernel
   quantum_gate
   classical_instructions
   platform
   compiler
   compiler_passes
   visualizer

.. toctree::
   :maxdepth: 3
   :caption: Tutorials

   qx_example
   dqcsim_example

.. toctree::
   :maxdepth: 1
   :caption: API reference

   python_api
   cpp_api

.. toctree::
   :maxdepth: 1
   :caption: Developer documentation

   coding_conventions
   doxygen_documentation

.. toctree::
   :maxdepth: 1
   :caption: Colophon

   changelog
   contributors

Indices and tables

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
