#!/bin/bash

mkdir -p build
rm ./wave

cmake --build ./build --config Debug --target all -j 8 --

cd build
./wave

cd ..