#!/bin/sh
# alternative to the --checkout option for `git submodule update`
# sets up shallow checkout of the `MyMod` directory in the repo

S=$(git rev-parse --git-dir)/modules/deps/eigen/info/sparse-checkout

#clear contents
: > $S

#include <unsupported/Eigen/MatrixFunctions>
# #include <Eigen/src/misc/lapacke.h>
echo 'unsupported/Eigen/MatrixFunctions' >>$S
echo 'unsupported/Eigen/src/MatrixFunctions' >>$S
echo 'Eigen/Core' >> $S
echo 'Eigen/LU' >> $S
echo 'Eigen/Eigenvalues' >> $S
echo 'Eigen/Householder' >> $S
echo 'Eigen/Cholesky' >> $S
echo 'Eigen/Jacobi' >> $S
echo 'Eigen/Geometry' >> $S
echo 'Eigen/SVD' >> $S
echo 'Eigen/QR' >> $S
echo 'Eigen/src/Core' >> $S
echo 'Eigen/src/plugins' >> $S
echo 'Eigen/src/LU' >> $S
echo 'Eigen/src/Eigenvalues' >> $S
echo 'Eigen/src/misc' >> $S
echo 'Eigen/src/Cholesky' >> $S
echo 'Eigen/src/Jacobi' >> $S
echo 'Eigen/src/Householder' >> $S
echo 'Eigen/src/Geometry' >> $S
echo 'Eigen/src/SVD' >> $S
echo 'Eigen/src/QR' >> $S

git -C deps/eigen config core.sparsecheckout true
git submodule update --force --checkout deps/eigen
