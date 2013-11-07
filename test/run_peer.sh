#!/bin/bash
export MAINDIR=../
export ETCDIR=etc
bin/$1 -i $2 -p ${MAINDIR}/nodes.map -c ${ETCDIR}/${3}.chunks -f ${ETCDIR}/${4}.masterchunks -m 4 -d 0
