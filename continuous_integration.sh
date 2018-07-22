#!/bin/sh
: ${NUMBER_OF_PROCESSORS:=72}
git submodule init
git submodule update
cd ${WORKSPACE}/src
make -j || exit 1
./test test.exe --gtest_output=xml:test_result.xml || exit 1
cd ${WORKSPACE}/scripts
for engine in ${ENGINES}
do
	python3 model_to_trace.py --binary_file_path ../src/${engine} \
		--input_model_directory_path ../data/problemsF \
		--output_trace_file_directory_path ../tmp/trace/${engine} \
		--output_info_file_directory_path ../tmp/info/${engine} \
		--output_energy_file_directory_path ../tmp/energy/${engine} \
		--timeout_sec 120 \
		--jobs ${NUMBER_OF_PROCESSORS} || exit 1
done
python3 compare_engine.py --default_info_directory_path ../resultF/default_info \
	--info_directory_path_base ../tmp/info \
	--engines "${ENGINES}" \
	--input_model_directory_path ../data/problemsF \
	--output_html_file_path compare_engine.html || exit 1
cp -f compare_engine.html /var/www/html/ || exit 1
