#!/bin/sh
g++ -Wall -O2 -c ../cowichan/cowichan.cpp
g++ -Wall -O2 -o cowichan_serial *.cpp cowichan.o
