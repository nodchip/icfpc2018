#!/bin/sh
export WORKSPACE=`pwd`
for engine in stupid_engine stupid_engine_v2 parallel_stupid_engine
do
	export BINARY_FILE_NAME=${engine}
	./execute_engine.sh || exit 1
done
cd result/trace
FILENAME=sanma`date +%Y%m%d%H%M%S`.zip
zip ${FILENAME} *.nbt
cd ${WORKSPACE}
mv result/trace/${FILENAME} .
sha256sum ${FILENAME}
