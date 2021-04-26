Welcome to OpenQL's documentation!
==================================

OpenQL is a framework for high-level quantum programming in C++/Python.
The framework provides a compiler for compiling and optimizing quantum code.
The compiler produces quantum assembly and instruction-level code for various target platforms.
While the instruction-level code is platform-specific, the quantum assembly code (QASM) is hardware-agnostic and can be simulated on one of the simulators.


.. toctree::
   :maxdepth: 2
   :caption: Getting started

   gs_overview
   gs_installation
   gs_start

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

   ex_qx
   ex_dqcsim

.. toctree::
   :maxdepth: 1
   :caption: Reference

   ref_python
   ref_cpp
   gen/ref_configuration
   gen/ref_architectures
   gen/ref_options
   gen/ref_passes
   gen/ref_resources

.. toctree::
   :maxdepth: 1
   :caption: Developer documentation

   dev_coding_conventions
   dev_doxygen_documentation

.. toctree::
   :maxdepth: 1
   :caption: Colophon

   col_changelog
   col_contributors

Indices and tables

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
