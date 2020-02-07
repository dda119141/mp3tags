#!/bin/bash

pushd 
cd out && rm -rf * && cmake -G "Unix Makefiles" ../ && make && ./tagwriter -s -o testclean.log
popd
