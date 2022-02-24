#!/bin/bash
make clean

make

./myproxy

while true
do
    sleep 1
done
