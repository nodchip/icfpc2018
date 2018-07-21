#!/bin/sh
git submodule init
git submodule update
cd ${WORKSPACE}/src
make -j || exit 1
./test test.exe --gtest_output=xml:test_result.xml || exit 1
cd ${WORKSPACE}/scripts
for engine in ${ENGINES}
do
	python3 model_to_trace.py --binary_file_path ../src/${engine} --input_model_directory_path ../data/problems --output_trace_file_directory_path ../tmp/trace --output_info_file_directory_path ../tmp/info --jobs 72 || exit 1
done
