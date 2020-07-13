#!/bin/bash
set -e

docker build --pull -t openql-manylinux ../.. -f Dockerfile
docker run openql-manylinux python/manylinux/helper.sh
docker cp openql-manylinux:/io/dist/ .
