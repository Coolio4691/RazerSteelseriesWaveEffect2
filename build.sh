#!/bin/bash

mkdir -p build
rm ./wave
cd build

cmake ..
make

./wave

cd ..