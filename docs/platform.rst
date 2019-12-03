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


A platform can be created in OpenQL by using ``Platform()`` API as show below:

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
