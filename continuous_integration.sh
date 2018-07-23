#!/bin/sh
: ${NUMBER_OF_PROCESSORS:=72}
export ENGINES=`cat engines.txt`
git submodule init
git submodule update
cd ${WORKSPACE}/src
make -j || exit 1
./test test.exe --gtest_output=xml:test_result.xml || exit 1
cd ${WORKSPACE}/scripts

python3 model_to_trace.py \
	--binary_file_names "${ENGINES}" \
	--binary_directory_path ../src \
	--input_model_directory_path ../data/problemsF \
	--output_trace_file_parent_directory_path ../tmp/trace \
	--output_info_file_parent_directory_path ../tmp/info \
	--output_energy_file_parent_directory_path ../tmp/energy \
	--timeout_sec 120 \
	--jobs ${NUMBER_OF_PROCESSORS} || exit 1
rm -rf ../tmp/trace || exit 1

python3 compare_engine.py \
	--default_info_directory_path ../resultF/default_info \
	--info_directory_path_base ../tmp/info \
	--engines "${ENGINES}" \
	--input_model_directory_path ../data/problemsF \
	--output_html_file_path compare_engine.html || exit 1
cp -f compare_engine.html /var/www/html/ || exit 1

# visualize
python3 visualize_compare_engine.py || exit 1
cp -f compare_engine_visualized.html /var/www/html/ || exit 1
cp -f compare_engine_visualized_*.png /var/www/html/ || exit 1
