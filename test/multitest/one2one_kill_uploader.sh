#!/bin/bash

export SPIFFY_ROUTER=127.0.0.1:12345

echo "start spiffy"

python ./run.py 0 &


echo "wait 5s to start peer 1 and peer 2"
echo "then kill peer 2 after 10s, and restart peer 2"

sleep 5

python ./runkill.py 2 &

sleep 3

echo "GET etc/B.chunks tmp/foo.foo" | ./peer -p nodes.map -i 1 -c etc/A.chunks -f etc/C.masterchunks -m 4 &

sleep 60

echo "have slept for 60s, hopefully it finishes downloading..."

echo "check sha1sum"

a=$(sha1sum tmp/foo.foo | awk '{ print $1 }')
b=$(sha1sum etc/B.tar | awk ' { print $1 }')

if [ "${a}" = "${b}" ];
then
echo "sha1sum checking OK!"
else
echo "sha1sum checking Failed!"
fi

exit 0
