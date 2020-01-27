Getting Started
===============


To begin working with OpenQL, you can start up python however you like. You can open a jupyter notebook (type ``jupyter notebook`` in your terminal), open an interactive python notebook in your terminal (with ``ipython3``), or simply launch python in your terminal (by typing ``python3``).

.. _helloworld:

Hello World
-----------

In this section we will run 'Hello World' example of OpenQL. The first step is to import openql which can be done by:

.. code:: python

    from openql import openql as ql


Next, create a platform

.. code:: python

	platform = ql.Platform("myPlatform", config_fn)

For this example we will be working on 3 qubits. So let us define a variable so that we can use it at multiple places in our code.

.. code:: python

    nqubits = 3


Create a program

.. code:: python

    p = ql.Program("aProgram", platform, nqubits)

Create a kernel

.. code:: python

    k = ql.Kernel("aKernel", platform, nqubits)

Populate kernel using default and custom gates

.. code:: python

    for i in range(nqubits):
        k.gate('prepz', [i])

    k.gate('x', [0])
    k.gate('h', [1])
    k.gate('cz', [2, 0])
    k.gate('measure', [0])
    k.gate('measure', [1])

Add the kernel to the program

.. code:: python

    p.add_kernel(k)

Compile the program

.. code:: python

    p.compile()


This will generate the output files in *test_output* directory.

A good place to get started with with your own programs might be to copy `examples/getting_started.py` to some folder of your choice and start modiifying it. For further examples, have a look at the test programs inside the "tests" directory.

.. todo::

    discuss the generated output files


Notebooks
---------

Following Jupyter notebooks are available in the ``<OpenQL Root Dir>/examples/notebooks`` directory:

ccLightClassicalDemo.ipynb
    This notebook provides an introduction to compilation for ccLight with an emphasis on:

    - hybrid quantum/classical code generation
    - control-flow in terms of:
        - if, if-else
        - for loop
        - do-while loop
    - getting measurement results


Examples
--------

Following Jupyter notebooks are available in the ``<OpenQL Root Dir>/examples`` directory:

getting_started.py
    The Hello World example discussed in helloworld_ section.

rb_single.py
    Single qubit randomized benchmarking.


Tests
-----

Various tests are also available in the ``<OpenQL Root Dir>/tests`` directory which can also be used as examples testing various features of OpenQL.
