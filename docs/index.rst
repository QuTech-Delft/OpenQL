Welcome to OpenQL's documentation!
==================================

.. warning::
   A major refactoring/revision of the documentation is currently underway, to
   synchronize with all the changes made as part of the modularization and
   refactoring effort. Many pages are still out of date; these pages are marked
   with a warning at the top. For up-to-date reference information about
   OpenQL, refer to the reference and developer documentation sections: these
   sections are largely generated from code, and are thus naturally in sync,
   but not all (reference) information from the "basics" section has been
   copied into it yet, and a proper "basics" section (i.e. one that doesn't
   rely on internal knowledge of OpenQL and/or require a compiler construction
   background) is yet to be written.

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

   dev_readme
   dev_build
   dev_automation
   dev_release
   dev_conventions
   dev_doxygen

.. toctree::
   :maxdepth: 1
   :caption: Colophon

   col_changelog
   col_contributors

Indices and tables

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
