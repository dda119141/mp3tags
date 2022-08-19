#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd "$SCRIPT_DIR"
cd out && rm -rf * && cmake -G "Unix Makefiles" ../ && make && ./tagwriter -s -o testclean.log
popd
