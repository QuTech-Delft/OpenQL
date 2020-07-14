This directory contains the build stuff needed to build and test manylinux2014
wheels for distribution through PyPI.

You need docker to do this, then you can just run `make` to build. The wheels
will end up in the `dist` folder, and will then be tested in a different
container (also based on manylinux) for all Python versions. Note that
qxelarator, libqasm, and qisa-assembler are NOT installed in the test
container, so a bunch of tests are skipped; this can't really be avoided until
they're appropriately built and published to PyPI as well. Otherwise we'd have
to replicate that entire process for just the test container, which is insane.

You need to `make clean` manually when any of the sources (other than tests)
change; there is no dependency detection.

manylinux2014 is used rather than the more compatible versions because OpenQL
refuses to build on manylinux2010, and I didn't bother trying manylinux1.
Apparently we're hitting some weird compiler bug on 2010 where two symbols just
go missing somewhere in the linking process.
