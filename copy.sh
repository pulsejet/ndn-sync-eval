#!/bin/bash

cp ~/mini-ndn/examples/svs.py .

rm -rf svs
cp -R ../svs/results svs
git -C ../../ndn-svs/ diff ndn-svs > svs/ndn-svs.patch
cp ../../ndn-svs/examples/* svs/

mkdir chronosync
git -C ../../ChronoSync/ diff src > chronosync/ChronoSync.patch
cp ../../ChronoSync/examples/* chronosync/
