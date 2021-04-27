#!/bin/bash

cp ~/mini-ndn/examples/svs.py .

cp ../results.ipynb .

rm -rf svs
mkdir svs
git -C ../../ndn-svs/ diff ndn-svs > svs/ndn-svs.patch
cp ../../ndn-svs/examples/* svs/

rm -rf chronosync
mkdir chronosync
git -C ../../ChronoSync/ diff src > chronosync/ChronoSync.patch
cp ../../ChronoSync/examples/* chronosync/

rm -rf psync
mkdir psync
git -C ../../PSync/ diff PSync > psync/PSync.patch
cp ../../PSync/examples/* psync/
