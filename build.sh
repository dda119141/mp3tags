#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd "$SCRIPT_DIR"
#cd out && make clean ../ && make && ./tagwriter
#cd out && make && ./tagwriter -s -o test.log
cd out && make
popd
