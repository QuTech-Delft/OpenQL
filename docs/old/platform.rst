.. _platform:

Platforms and architectures
===========================

OpenQL supports various target platforms. These platforms can be software simulators or architectures targetting hardware quantum computers.

In principle, platforms are described entirely via a JSON configuration file; as few architecture-dependent things as possible are actually hardcoded in OpenQL. This means, however, that the platform configuration file is quite extensive, making the learning curve for making one from scratch or even adjusting one pretty steep. Furthermore, actually compiling for a particular platform may also require a custom compilation strategy (i.e. which steps/passes the compiler takes/does to actually compile the program), further complicating things. Therefore, OpenQL ships with a bunch of logic to generate sane default settings for particular architectures that we support out of the box. These architectures, and the defaults they provide, are listed in the :ref:`ref_architectures` section.

A platform can be created in OpenQL by using one of the various constructors for the ``ql.Platform()`` class. The most commonly used way to do obtain a platform object is as follows:

.. code:: python

    platform = ql.Platform('<platform_name>', `<platform_config>`)

Here, ``<platform_name>`` is anything you want it to be (it's only used for logging) and ``<platform_config>`` can be a recognized architecture name, such as `"none"` or `"cc"`, or it can point to a platform configuration file. For example, a platform with the name ``CCL_platform`` that uses the defaults for CC-light can be created as:

.. code:: python

    platform = ql.Platform('CCL_platform', 'cc_light')

For more details, see also the :ref:`ref_python`, :ref:`ref_configuration`, and :ref:`ref_architectures` sections.
