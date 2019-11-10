.. OpenQL documentation master file, created by
   sphinx-quickstart on Sun Nov  3 17:43:19 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

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

.. scheduling
.. classical_instructions
.. backends
.. config_file
.. changelog
.. api


.. toctree::
   :maxdepth: 1
   :caption: API Reference

.. apidocs/gate
.. apidocs/kernel
.. apidocs/program
.. apidocs/platform

   


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
