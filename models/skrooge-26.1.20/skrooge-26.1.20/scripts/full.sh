#!/bin/bash
make install -j4
make apidox
make test
make codecheck
