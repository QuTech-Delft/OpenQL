#!/bin/sh
# alternative to the --checkout option for `git submodule update`
# sets up shallow checkout of the `MyMod` directory in the repo

S=$(git rev-parse --git-dir)/info/sparse-checkout
echo 'Eigen/*'  >>$S
echo 'unsupported/*' >>$S
git config core.sparsecheckout true
git checkout --force $1
