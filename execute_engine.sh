#!/bin/sh
: ${NUMBER_OF_PROCESSORS:=72}
git submodule init
git submodule update
cd ${WORKSPACE}/src || exit 1
make -j ${NUMBER_OF_PROCESSORS} || exit 1
cd ${WORKSPACE}/scripts || exit 1
python3 model_to_trace.py \
        --binary_file_names ${BINARY_FILE_NAME} \
        --binary_directory_path ../src \
        --input_model_directory_path ../data/problemsF \
        --output_trace_file_parent_directory_path ../tmp/trace \
        --output_info_file_parent_directory_path ../tmp/info \
        --output_energy_file_parent_directory_path ../tmp/energy \
		--timeout_sec 120 \
        --jobs ${NUMBER_OF_PROCESSORS} || exit 1
python3 update_result.py --temp_trace_directory_path ../tmp/trace/${BINARY_FILE_NAME} \
        --temp_info_directory_path ../tmp/info${BINARY_FILE_NAME} \
        --result_trace_directory_path ../resultF/trace \
        --result_info_directory_path ../resultF/info || exit 1
rm -Rf ../tmp/trace/${BINARY_FILE_NAME}
