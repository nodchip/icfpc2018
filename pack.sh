#!/bin/sh
export WORKSPACE=`pwd`
export ENGINES=`cat engines.txt`

git submodule init
git submodule update

cd ${WORKSPACE}/src || exit 1
make -j ${NUMBER_OF_PROCESSORS} || exit 1
cd ${WORKSPACE}/scripts || exit 1
python3 update_result.py --temp_trace_directory_path ../data/by_hand/trace \
        --temp_info_directory_path ../data/by_hand/info \
        --result_trace_directory_path ../resultF/trace \
        --result_info_directory_path ../resultF/info || exit 1
cd ${WORKSPACE}

for engine in ${ENGINES}
do
	export BINARY_FILE_NAME=${engine}
	./execute_engine.sh || exit 1
done
cd resultF/trace
FILENAME=sanma`date +%Y%m%d%H%M%S`.zip
zip ${FILENAME} *.nbt
cd ${WORKSPACE}
mv resultF/trace/${FILENAME} .
sha256sum ${FILENAME}
