#!/bin/bash
g++ -o libusage.so -O3 -fPIC -Wl,-h -Wl,libusage.so -shared usage.cc
