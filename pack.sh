#!/bin/sh
export WORKSPACE=`pwd`
for engine in stupid_engine
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
