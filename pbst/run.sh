#!/usr/bin/bash

PARALLELISM=$1
X=$2
DURATION=$3

rm -rf build
mkdir build
cd build
cmake ..
make

for i in $(seq 1 5);
do
    echo "Experiment #$i:"
    ./pbst $PARALLELISM $X $DURATION
    echo ""
done
cd ..
rm -rf build
