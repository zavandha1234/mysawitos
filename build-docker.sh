#!/bin/bash
# Build ISO lewat Docker (Linux/Mac/Windows+Docker Desktop). Hasil: MySawitOS-v1.0.iso
set -e
docker build -t mysawit-build .
docker run --rm -v "$PWD":/src mysawit-build
ls -lh MySawitOS-v1.0.iso
