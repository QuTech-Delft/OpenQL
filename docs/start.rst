Getting Started
===============


To begin, start up python however you like. You can open a jupyter notebook (type ``jupyter notebook`` in your terminal),
open an interactive python notebook in your terminal (with ``ipython3``), or simply launch python in your terminal (type ``python3``)

.. code:: python

    from openql import openql as ql


Next, create a platform

.. code:: python

	platform = ql.Platform("myPlatform", config_fn)

For this example we will be working on 3 qubits. So let us define variable so that we can use it at multiple places in our code.

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