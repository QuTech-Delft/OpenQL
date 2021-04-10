#!/bin/bash
set -e

if [[ -z "$AUDITWHEEL_PLAT" ]]; then
    echo "This should only be run inside the container!"
    exit 1
fi

set -u
set -x

for PYDIR in /opt/python/cp36-cp36m /opt/python/cp37-cp37m /opt/python/cp38-cp38 /opt/python/cp39-cp39; do
    rm -rf pybuild/cbuild/python pybuild/dist
    "${PYDIR}/bin/python" setup.py bdist_wheel
    ${PYBIN}/python -m auditwheel repair -w /io/dist pybuild/dist/*.whl
done
