#!/usr/bin/bash

rm -rf build
mkdir build
cd build
cmake ..
make
for i in $(seq 1 5);
do
    echo "Running experiment #$i..."
    ./pqsort
    echo ""
done
cd ..
rm -rf build
