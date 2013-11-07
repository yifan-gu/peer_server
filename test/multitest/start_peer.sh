#!/bin/bash
echo "$*"
./peer -p nodes.map -i $1 -c etc/${2}.haschunks -f etc/C.masterchunks -m 4
