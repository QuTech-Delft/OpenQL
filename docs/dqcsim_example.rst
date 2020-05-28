DQCsim Simulation
=================

This tutorial modifies the QX simulation tutorial to use `DQCsim <https://qe-lab.github.io/dqcsim/>`__. In short, DQCsim is a framework that allows simulations to be constructed by chaining plugins operating on a stream of gates and measurement results, thus making it easier to play around with error models, gather runtime statistics, and connect different quantum simulators to different algorithm file formats. In this tutorial, we will use it to simulate the toy example modelling an 8-faced die with QX and QuantumSim's error models.

Note that DQCsim currently does not work on Windows. If you're using a Windows workstation, you'll need to work in a virtual machine or on a Linux server.

Dependencies
------------

DQCsim and the plugins we'll be using can be installed using pip as follows:

.. code::

    python -m pip install dqcsim dqcsim-qx dqcsim-quantumsim dqcsim-cqasm


You'll probably need to prefix `sudo` to make that work, and depending on your Linux distribution you may need to substitute `python3`. If you don't have superuser access, you can add the `--user` flag, but you'll need to make sure that DQCsim's executables are in your system path. The easiest way to do that is figure out the path using `python -m pip uninstall dqcsim`, observe the directory that the `bin/dqcsim` file lives in, and add that to your path using `export PATH=$PATH:...`, replacing the `...` with the listed path from `/` to `bin`.

We'll also need to add some modules to the Python file from the QX die example:

.. code:: python

    from dqcsim.host import *
    import shutil


Replicating the QXelarator results
----------------------------------

The results we got when using QX directly are pretty easy to replicate. Here's how:

.. code:: python

    def dice_execute_singleshot():
        print('executing 8-face dice program on DQCsim using QX')

        # DQCsim disambiguates between input file formats based on file extension.
        # .qasm is already in use for OpenQASM files, so DQCsim uses .cq for cQASM
        shutil.copyfile('test_output/dice.qasm', 'test_output/dice.cq')

        # open the simulation context and run the simulation. the cQASM frontend
        # returns the results as a JSON object for us to parse througn run()
        with Simulator('test_output/dice.cq', 'qx') as sim:
            results = sim.run()

        # parse the measurement results
        res = [results['qubits'][q]['value'] for q in range(nqubits)]

        # convert the measurement results from 3 qubits to dice face value
        dice_face = reduce(lambda x, y: 2*x+y, res, 0) +1
        print('Dice face : {}'.format(dice_face))


The key is the `Simulator('test_output/dice.cq', 'qx')` expression wrapped in the `with` block, which constructs a DQCsim simulation using the `cq` frontend (based on the file extension, that's why we have to make a copy and rename OpenQL's output first) and the `qx` backend, wrapping the libqasm cQASM parser and QX's internals respectively.

Enabling QX's depolarizing channel error model
----------------------------------------------

While not exactly useful for this particular algorithm, we can use DQCsim to enable QX's error model without having to edit the cQASM file. The easiest way to do that is to add a line before `sim.run()` to form

.. code:: python

    with Simulator('test_output/dice.cq', 'qx') as sim:
        sim.arb('back', 'qx', 'error', model='depolarizing_channel', error_probability=0.2)
        results = sim.run()


This requires some explanation. The `sim.arb()` function (docs `here <https://qe-lab.github.io/dqcsim/py_/dqcsim/host/index.html#dqcsim.host.Simulator.arb>`__) instructs DQCsim to send a so-called ArbCmd (short for "arbitrary command") to one of its plugins. In short, ArbCmds are DQCsim's way to let its users communicate intent between plugins, without DQCsim itself needing to know what's going on. DQCsim has no concept of error models and the likes built-in, so we need to use ArbCmds to configure them.

Its first argument specifies the plugin that the ArbCmd is intended for, where `'back'` is simply the default name for the backend plugin. You could also use the integer 1 to select the second plugin from the front, or -1 to select the first plugin from the back, as if it's indexing a Python list.

The second and third argument specify the interface and operation identifiers respectively. The interface identifier is usually just the name of the plugin, acting like a namespace or the name of a class, while the operation identifier specifies what to do, acting as a function or method name. You'll have to read the `plugin documentation <https://github.com/QE-Lab/dqcsim-qx>`__ to see which interface/operation pairs are supported. Usually these are listed in the form `<interface>.<operation>`, as if we're using a parameter named `<operation>` from a class named `<interface>`.

Note that the semantics of ArbCmds are defined such that plugins will happily ignore any ArbCmd specifying an interface they don't support, but will complain when they support the interface but don't understand the operation. More information and the rationale for this can be found `here <https://qe-lab.github.io/dqcsim/intro/arbs.html>`__.

Any remaining arguments are interpreted as arguments. Specifically, keyword arguments are transformed into the keys and values of a JSON object, in this case `{"model": "depolarizing_channel", "error_probability": 0.2}`. Positional arguments are interpreted as binary strings, but those are out of the scope of this tutorial (they're not that relevant in the Python world). Again, you'll have to read the plugin documentation to see what arguments are expected.

You won't be able to see much in the result of the algorithm, because it was already purely random. But you may notice that the log output of DQCsim now includes a `Depolarizing channel model inserted ... errors` from the backend.

Using QuantumSim instead
------------------------

More interesting in terms of DQCsim's functionality is just how easy it is to change the simulator. All you have to do to simulate using QuantumSim instead of QX is change the `'qx'` in the `Simulation` constructor with `'quantumsim'`.

While QuantumSim is capable of much more, its `DQCsim plugin <https://github.com/jvanstraten/dqcsim-quantumsim>`__ currently only supports a qubit error model based on t1/t2 times. The arb for that, along with the modified `Simulator` constructor, looks like this:

.. code:: python

    with Simulator('test_output/dice.cq', 'quantumsim') as sim:
        sim.arb('back', 'quantumsim', 'error', t1=10.0, t2=20.0)
        results = sim.run()


For that to have any merit whatsoever, you'll have to modify the code such that we're at least simulating OpenQL's scheduled output, because it's based entirely on the timing of the circuit:

.. code:: python

    shutil.copyfile('test_output/dice_scheduled.qasm', 'test_output/dice.cq')


One thing the QuantumSim plugin does that the QX plugin doesn't is report the actual probability of a qubit measurement result. The `results` variable looks like this:

.. code:: json

    {
      "qubits": [
        {
          "value": 0,
          "raw": 0,
          "average": 0.0,
          "json": {"probability": 0.5},
          "binary": [[0, 0, 0, 0, 0, 0, 224, 63]]
        },
        {
          "value": 0,
          "raw": 0,
          "average": 0.0,
          "json": {"probability": 0.5},
          "binary": [[0, 0, 0, 0, 0, 0, 224, 63]]
        },
        {
          "value": 0,
          "raw": 0,
          "average": 0.0,
          "json": {"probability": 0.5},
          "binary": [[0, 0, 0, 0, 0, 0, 224, 63]]
        }
      ]
    }

In particular, the `"json"` parameter lists data that the cQASM frontend received from the backend but doesn't know about, in this case showing that the probability for this outcome was exactly 0.5 for each of the three individual measurements.

Further reading
---------------

A more extensive Python tutorial for DQCsim can be found `here <https://qe-lab.github.io/dqcsim/python-api/index.html>`__. It (intentionally) does not depend on any of the plugins and doesn't use OpenQL, but hopefully the above illustrates that swapping out plugins is about the easiest thing you can do with DQCsim.
