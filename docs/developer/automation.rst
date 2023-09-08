.. _dev_automation:

Build automation
================

OpenQL employs continuous integration based on GitHub Actions,
to ensure that new features or modifications actually work on all supported systems.
The files to this end are in the ``.github`` directory of the repository.
Furthermore, ReadTheDocs can build and publish documentation for OpenQL automatically,
for which the files are in the ``docs`` folder (specifically,
it uses the ``docs/.conda.yml`` file for the Conda environment and then just runs the makefile in ``docs``).


Integration tests
-----------------

The integration tests are run when you push to a branch for which a pull request is open, or when ``develop`` changes:

- C++ test suite (tests and examples).
   - x64 (GitHub runner):
      - ``ubuntu-latest``, ``g++/clang++``, ``Debug/Release``.
      - ``macos-latest``, ``clang++``, ``Debug/Release``.
      - ``windows-latest``, ``msvc``, ``Debug/Release``.
   - ARM (self-hosted runner):
      - ``Linux``, ``g++``, ``Release``.
      - ``MacOS``, ``clang++``, ``Release``.

- Python test suite.
   - x64 (GitHub runner):
      - ``ubuntu-latest/macos-latest/windows-latest``, ``Python 3.11``.


.. note::
   To test an incomplete branch that you're still working on, please open a draft pull request.


Release automation
------------------

Release artifact generation triggers on a push to a branch that starts with ``release_`` (used for testing)
or when a new release is made via GitHub and/or a tag is pushed.

.. warning::
   Please don't do any of these things until you have read the :ref:`dev_release`!
