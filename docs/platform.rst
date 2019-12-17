.. _platform:

Platform
========

OpenQL supports various target platforms.
These platforms can be software simulators or architectures targetting hardware quantum computers.
The following platforms are supported by OpenQL:

- software simulator platforms
	- :ref:`qxplatform`
	- :ref:`qsimplatform`
- hardware platforms
	- :ref:`cclplatform`
	- :ref:`ccplatform`
	- :ref:`cboxplatform`

:Note: Quantumsim and QX are not really platforms. They are means to simulate a particular (hardware) platform. In the mapper branch quantumsim code generation is controlled by an option.  This option is checked at the end of the platform independent compiler and near the end of the CC-Light backend, and will cause quantumsim to simulate the circuit before and after mapping and rcscheduling for CC-Light, respectively.

:Note: We are planning to use DQCsim, a platform to connect to simulators. In that context, software simulator platforms are connected to by DQCsim, and OpenQL just provides compilation support to a particular hardware platform.

A platform can be created in OpenQL by using ``Platform()`` API as shown below:

.. code:: python

    platform = ql.Platform('<platform_name>', <path_to_json_config_file>)

For example,
a platform with the name ``CCL_platform`` and using ``hardware_config_cc_light.json`` as platform configuration file
can be created as:

.. code:: python

    platform = ql.Platform('CCL_platform', 'hardware_config_cc_light.json')


Platform Configuration File
---------------------------

The platform configuration file describes the target platform in JSON format. Developers can use this file to:

- specify the backend compiler to be used for this platform
- specify hardware settings related to control electronics in the experiments (for hardware backends)
- specify the scheduling resources such as qubits, waveform generators, measuring units, and the qubit topology of the platform
- specify the list of instructions supported by the platform
- specify gate decomposition



.. include:: platform_qx.rst
.. include:: platform_quantumsim.rst
.. include:: platform_ccl.rst
.. include:: platform_cc.rst
.. include:: platform_cbox.rst
