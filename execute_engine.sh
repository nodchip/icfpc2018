#!/bin/sh
: ${NUMBER_OF_PROCESSORS:=72}
git submodule init
git submodule update
cd ${WORKSPACE}/src || exit 1
make -j ${NUMBER_OF_PROCESSORS} || exit 1
cd ${WORKSPACE}/scripts || exit 1
python3 model_to_trace.py \
        --binary_file_path ../src/${BINARY_FILE_NAME} \
        --input_model_directory_path ../data/problemsF \
        --output_trace_file_directory_path ../tmp/trace \
        --output_info_file_directory_path ../tmp/info \
        --output_energy_file_directory_path ../tmp/energy \
		--timeout_sec 120 \
        --jobs ${NUMBER_OF_PROCESSORS} || exit 1
python3 update_result.py --temp_trace_directory_path ../tmp/trace \
        --temp_info_directory_path ../tmp/info \
        --result_trace_directory_path ../result/trace \
        --result_info_directory_path ../result/info || exit 1
