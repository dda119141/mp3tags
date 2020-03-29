#!/bin/bash

pushd .
#cd out && make clean ../ && make && ./tagwriter
cd out && make && ./tagwriter -s -o test.log
popd
