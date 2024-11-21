#!/usr/bin/bash

PARALLELISM=$1

rm -rf build
mkdir build
cd build
cmake ..
make

echo ""
echo "Running experiments with parallelsm=$PARALLELISM"
echo ""
for i in $(seq 1 5);
do
    echo "Experiment #$i:"
    ./pbfs $PARALLELISM
    echo ""
done
cd ..
rm -rf build
