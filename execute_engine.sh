#!/bin/sh
git submodule init
git submodule update
cd ${WORKSPACE}/src || exit 1
make -j || exit 1
cd ${WORKSPACE}/script || exit 1
python model_to_trace.py --binary_file_path ../src/${BINARY_FILE_NAME} --input_model_directory_path ../data/problems --output_trace_file_directory_path ../tmp/trace --output_info_file_directory_path ../tmp/info --jobs 72 || exit 1
python update_result.py --temp_trace_directory_path ../tmp/trace --temp_info_directory_path ../tmp/info --result_trace_directory_path ../result/trace --result_info_directory_path ../result/info || exit 1
