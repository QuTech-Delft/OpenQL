.. _ref_cpp:

C++ API
=======

If you're more of a C++ than a Python person, the same API exposed to Python can also be used from within C++.

There is currently no supported way to install OpenQL as a system library. Instead, you can use CMake to include OpenQL as a dependency of your program. This is pretty straight-forward:

.. literalinclude:: ../examples/cpp-standalone-example/CMakeLists.txt
   :language: cmake

With that configuration, ``#include <openql>`` becomes available, which places the OpenQL API in the ``ql`` namespace. Here's a basic example of what a program might look like:

.. literalinclude:: ../examples/cpp-standalone-example/example.cc
   :language: c++

The API is documented `here <doxy/namespaceql_1_1api.html>`_.

.. note::

   The API classes are merely wrappers of the classes used internally by OpenQL. You can of course also use the internal classes, but their interfaces should not be assumed to be stable from version to version.
