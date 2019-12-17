.. _qsimplatform:

Quantumsim Platform Configuration
---------------------------------

Details of the configuration file for the Quantumsim simulator platform. [TBD]

:Note: Quantumsim is not really a platform. It is a means to simulate a particular (hardware) platform. In the mapper branch quantumsim code generation is controlled by an option. When this option doesn't have the value `no`, it is expected to indicate for which quantumsim code generation is to be done; subsequently, generation of that code is done to an output file that is to be used when calling quantumsim. The quantumsim option is checked in the CC-Light backend and will cause quantumsim to simulate the circuit after mapping and rcscheduling for CC-Light.
