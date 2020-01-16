Welcome to OpenQL's documentation!
==================================

OpenQL is a framework for high-level quantum programming in C++/Python. The framework provides a compiler for compiling and optimizing quantum code. The compiler produces the intermediate quantum assembly language and the compiled micro-code for various target platforms. While the microcode is platform-specific, the quantum assembly code (qasm) is hardware-agnostic and can be simulated on the QX simulator.


.. toctree::
   :maxdepth: 2
   :caption: Contents:

   installation
   start
   changelog
   contributors

.. toctree::
   :maxdepth: 3
   :caption: OpenQL Basics:

   platform
   quantum_gate
   classical_instructions
   kernel
   program
   compiler_passes

.. toctree::
   :maxdepth: 1
   :caption: API Reference

   api



.. .. toctree::
..    :maxdepth: 1
..    :caption: API Reference

..    api/openql
..    api/kernel
..    api/program
..    api/platform
..    api/operation
..    api/creg



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
