# Manylinux version to use.
ARG MANYLINUX=2014
FROM quay.io/pypa/manylinux${MANYLINUX}_x86_64

# Install dependencies from PyPI for all Python versions.
RUN for PYDIR in /opt/python/cp3*; do               \
        "${PYDIR}/bin/pip" install pytest numpy;    \
    done

WORKDIR /test

ADD . .
