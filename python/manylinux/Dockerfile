ARG MANYLINUX=2010
FROM quay.io/pypa/manylinux${MANYLINUX}_x86_64

ARG PYTHON_VERSION=36
ENV PYBIN /opt/python/cp${PYTHON_VERSION}-cp${PYTHON_VERSION}*/bin

RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2-Linux-x86_64.tar.gz | tar xz -C /usr/ --strip-components=1 && \
${PYBIN}/pip3 install -U pip auditwheel==3.0.0 && \
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
patch /opt/_internal/cpython-3.6.10/lib/python3.6/site-packages/auditwheel/repair.py auditwheel.patch && \
mkdir -p flex && curl -L https://github.com/westes/flex/files/981163/flex-2.6.4.tar.gz | tar xz -C /flex/ --strip-components=1 && \
cd flex && ./configure && make -j && make install && cd .. && \
mkdir -p bison && curl -L ftp://ftp.gnu.org/gnu/bison/bison-3.5.tar.gz | tar xz -C /bison/ --strip-components=1 && \
cd bison && ./configure && make -j && make install && cd ..

WORKDIR /src

ADD . .

# ENTRYPOINT ["bash", "-c", "\
#     $NPROCS=4 {PYBIN}/python3 setup.py bdist_wheel && \
#     LD_LIBRARY_PATH=/opt/python/cp36-cp36m/lib AUDITWHEEL_MOD_PYVER=cp35 ${PYBIN}/python3 -m auditwheel repair -w /io/dist target/python/dist/*.whl && \
#     sleep 2 && \
#     LD_LIBRARY_PATH=/opt/python/cp36-cp36m/lib AUDITWHEEL_MOD_PYVER=cp36 ${PYBIN}/python3 -m auditwheel repair -w /io/dist target/python/dist/*.whl && \
#     sleep 2 && \
#     LD_LIBRARY_PATH=/opt/python/cp36-cp36m/lib AUDITWHEEL_MOD_PYVER=cp37 ${PYBIN}/python3 -m auditwheel repair -w /io/dist target/python/dist/*.whl && \
#     sleep 2 && \
#     LD_LIBRARY_PATH=/opt/python/cp36-cp36m/lib AUDITWHEEL_MOD_PYVER=cp38 ${PYBIN}/python3 -m auditwheel repair -w /io/dist target/python/dist/*.whl"]
