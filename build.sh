#!/bin/bash

pushd . 
#cd out && make clean ../ && make && ./tagreader
cd out && make && ./tagreader
popd
