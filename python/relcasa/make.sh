#!/bin/sh
make -j
./prep/Prep > ../src/main.cc
make -j
rm -f RelCASA*
./SMITH3
./header_split.py
./tasks_split.py
./gen_split.py
./queue_split.py
./gamma.py
