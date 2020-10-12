#!/bin/bash
set -e

rm -rf ../dist
mkdir -p ../dist
docker build --pull -t openql-manylinux ../../.. -f Dockerfile
docker run -v `pwd`/../dist:/io/dist openql-manylinux python/manylinux/build/helper.sh
