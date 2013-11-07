#!/bin/bash

export SPIFFY_ROUTER=127.0.0.1:12345

echo "start spiffy"

python ./run.py 0 &

sleep 1

echo "wait 5s to start peers"

sleep 5

./start_peer.sh 9 C &

./start_peer.sh 8 C &

./start_peer.sh 10 C &

./start_peer.sh 11 C &


sleep 1

echo "ready to start downloading peers"

sleep 5

echo "GET etc/C.chunks tmp/foo.foo" | ./peer -p nodes.map -i 1 -c etc/A.chunks -f etc/C.masterchunks -m 3 &

sleep 80

echo "have slept for 60s, hopefully it finishes downloading..."

sleep 1

echo "check sha1sum"

sleep 1

a=$(sha1sum tmp/foo.foo | awk '{ print $1 }')
b=$(sha1sum etc/C.tar | awk ' { print $1 }')

if [ "${a}" = "${b}" ];
then
echo "sha1sum checking OK!"
else
echo "sha1sum cheching Failed!"
fi

exit 0



