#!/bin/bash
set -e

if [[ -z "$AUDITWHEEL_PLAT" ]]; then
    echo "This should only be run inside the container!"
    exit 1
fi

set -u
set -x

for PYDIR in /opt/python/cp3*; do
    "${PYDIR}/bin/pip" install --no-index -f /io/dist qutechopenql
    "${PYDIR}/bin/python" -m pytest
done
