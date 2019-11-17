#!/bin/bash

pushd . 
cd out && make clean ../ && make && ./tagreader
popd
