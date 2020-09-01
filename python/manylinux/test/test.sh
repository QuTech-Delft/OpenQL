#!/bin/bash
set -e

docker build --pull -t openql-manylinux-test ../../.. -f Dockerfile
docker run -v `pwd`/../dist:/io/dist:ro openql-manylinux-test python/manylinux/test/helper.sh
