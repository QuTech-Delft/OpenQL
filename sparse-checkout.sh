#!/bin/sh
# alternative to the --checkout option for `git submodule update`
# sets up shallow checkout of the `MyMod` directory in the repo

S=$(git rev-parse --git-dir)/modules/deps/eigen/info/sparse-checkout
echo 'Eigen/*'  >>$S
echo 'unsupported/*' >>$S
git -C deps/eigen config core.sparsecheckout true
git submodule update --force --checkout deps/eigen
