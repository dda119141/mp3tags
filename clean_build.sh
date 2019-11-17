#!/bin/bash

pushd 
cd out && rm -rf * && cmake ../ && make && ./tagreader
popd
