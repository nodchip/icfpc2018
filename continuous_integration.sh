#!/bin/sh
git submodule init
git submodule update
cd ${WORKSPACE}/src
make -j || exit 1
./test test.exe --gtest_output=xml:test_result.xml || exit 1
for engine in ${ENGINES}
do
    ./${engine} --model ../data/problems/LA001_tgt.mdl --engine default --info output.json --trace-output LA001_out.nbt || exit 1
done
