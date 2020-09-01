# Manylinux version to use.
ARG MANYLINUX=2014
FROM quay.io/pypa/manylinux${MANYLINUX}_x86_64

# Max number of parallel processes to use.
ARG NPROCS=100
ENV NPROCS ${NPROCS}

# Install flex from source (dependency for libqasm and tree-gen).
RUN mkdir -p flex       && \
    curl -L https://github.com/westes/flex/files/981163/flex-2.6.4.tar.gz | tar xz -C /flex/ --strip-components=1 && \
    cd flex             && \
    ./configure         && \
    make -j $NPROCS     && \
    make install        && \
    cd ..               && \
    rm -rf flex

# Install bison from source (dependency for libqasm and tree-gen).
RUN mkdir -p bison      && \
    curl -L ftp://ftp.gnu.org/gnu/bison/bison-3.5.tar.gz | tar xz -C /bison/ --strip-components=1 && \
    cd bison            && \
    ./configure         && \
    make -j $NPROCS     && \
    make install        && \
    cd ..               && \
    rm -rf bison

# Install glpk from source (dependency for lemon, which is itself a submodule).
RUN mkdir -p glpk       && \
    curl -L https://ftp.gnu.org/gnu/glpk/glpk-4.35.tar.gz | tar xz -C /glpk/ --strip-components=1 && \
    cd glpk             && \
    ./configure         && \
    make -j $NPROCS     && \
    make install        && \
    cd ..               && \
    rm -rf glpk

# Install pcre from source (dependency for swig).
RUN mkdir -p pcre       && \
    curl -L ftp://ftp.pcre.org/pub/pcre/pcre-8.41.tar.bz2 | tar xj -C /pcre/ --strip-components=1 && \
    cd pcre             && \
    ./configure         && \
    make -j $NPROCS     && \
    make install        && \
    cd ..               && \
    rm -rf pcre

# Install swig from source (python binding generator).
RUN mkdir -p swig       && \
    curl -L https://netix.dl.sourceforge.net/project/swig/swig/swig-3.0.12/swig-3.0.12.tar.gz | tar xz -C /swig/ --strip-components=1 && \
    cd swig             && \
    ./configure         && \
    make -j $NPROCS     && \
    make install        && \
    cd ..               && \
    rm -rf swig

# Install auditwheel and patch it with some fixes.
ARG PYTHON_VERSION=36
ENV PYBIN /opt/python/cp${PYTHON_VERSION}-cp${PYTHON_VERSION}*/bin
RUN ${PYBIN}/pip3 install -U pip auditwheel==3.0.0 && \
    echo "214a215" > auditwheel.patch && \
    echo ">         remove_platforms = list(remove_platforms)" >> auditwheel.patch && \
    echo "225a227,229" >> auditwheel.patch && \
    echo ">     mod_pyver = os.environ.get('AUDITWHEEL_MOD_PYVER', None)" >> auditwheel.patch && \
    echo ">     if mod_pyver:" >> auditwheel.patch && \
    echo ">         fparts['pyver'] = mod_pyver" >> auditwheel.patch && \
    patch /opt/_internal/cpython-3.6.10/lib/python3.6/site-packages/auditwheel/policy/policy.json auditwheel.patch && \
    echo '74a75,80' > auditwheel.patch && \
    echo ">             elif pkg_root.endswith('.data'):" >> auditwheel.patch && \
    echo '>                 # If this is a file in the .data section of the wheel, using' >> auditwheel.patch && \
    echo '>                 # .libs will not work. In order to not make assumptions about' >> auditwheel.patch && \
    echo '>                 # the data dir we place the libs in a subdir of where the' >> auditwheel.patch && \
    echo '>                 # binary resides, named `<binary>.libs`.' >> auditwheel.patch && \
    echo ">                 dest_dir = pjoin(fn + '.libs')" >> auditwheel.patch && \
    patch /opt/_internal/cpython-3.6.10/lib/python3.6/site-packages/auditwheel/repair.py auditwheel.patch

# Install CMake from a binary package.
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.1.3/cmake-3.1.3-Linux-x86_64.tar.gz | tar xz -C /usr/ --strip-components=1
# RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.16.0/cmake-3.16.0-Linux-x86_64.tar.gz | tar xz -C /usr/ --strip-components=1

WORKDIR /src

ADD . .
