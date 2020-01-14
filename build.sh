#!/bin/bash

pushd . 
#cd out && make clean ../ && make && ./tagreader
cd out && make && ./tagreader
./tagwriter -s -o test.log
popd
