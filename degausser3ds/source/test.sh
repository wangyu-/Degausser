#! /bin/sh
rm a.bin
g++ -DTEST bbp_process.cpp comm.cpp miniz.c -o a.bin
./a.bin
