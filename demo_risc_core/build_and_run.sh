#!/bin/sh
make clean all
rm -rf *.bin
python3 compile.py -i $1
./demo_risc_core out.bin
