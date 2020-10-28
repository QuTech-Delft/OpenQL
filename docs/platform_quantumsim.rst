.. _qsimplatform:

Quantumsim Platform
-------------------

The OpenQL compiler is able to generate a python script to interface to quantumsim.
This script generation is controlled by option *quantumsim*:

- no:
  No script to interface to quantumsim is generated.

- yes:
  A python script is generated to interface with a standard version of quantumsim.

- qsoverlay:
  A python script is generated to interface with the qsoverlay module on top of quantumsim.

The scripts are generated in the default output directory.

Unlike in previous releases, quantumsim is not considered a platform.
It is a means to simulate a particular (hardware) platform.

The quantumsim option is checked in the CC-Light backend in two places:

- just when entering the backend after *decomposition before scheduling*

- just before generating QISA, i.e. after *mapping*, *rcscheduling* and *decomposition after scheduling*.

[TBD]
