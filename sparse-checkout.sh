#!/bin/sh
# alternative to the --checkout option for `git submodule update`
# sets up shallow checkout of the `MyMod` directory in the repo

git config submodule.deps/eigen.update '!../update.sh'
S=$(git rev-parse --git-dir)deps/eigen/info/sparse-checkout
echo 'deps/eigen/Eigen/*'  >>$S
echo 'deps/eigen/unsupported/*' >>$S
git -C deps/eigen config core.sparsecheckout true
git checkout --force $1
