#!/bin/bash

export SPIFFY_ROUTER=127.0.0.1:12345

echo "start spiffy"

python ./run.py 0 &

sleep 1

echo "wait 5s to start peers"

sleep 5

./peer -p nodes.map -i 2 -c etc/C.chunks -f etc/C.masterchunks -m $1 &

sleep 1

echo "ready to start uploading peers"

sleep 5

echo "GET etc/B.chunks tmp/foo.foo8" | ./peer -p nodes.map -i 8 -c etc/A.chunks -f etc/C.masterchunks -m 3 -n &

echo "GET etc/B.chunks tmp/foo.foo9" | ./peer -p nodes.map -i 9 -c etc/A.chunks -f etc/C.masterchunks -m 3 -n &

echo "GET etc/B.chunks tmp/foo.foo10" | ./peer -p nodes.map -i 10 -c etc/A.chunks -f etc/C.masterchunks -m 3 -n &

echo "GET etc/B.chunks tmp/foo.foo11" | ./peer -p nodes.map -i 11 -c etc/A.chunks -f etc/C.masterchunks -m 3 -n &

sleep 80

echo "have slept for 60s, hopefully it finishes downloading..."

sleep 1

echo "check sha1sum"

sleep 1

a=$(sha1sum tmp/foo.foo8 | awk '{ print $1 }')
b=$(sha1sum etc/B.tar | awk ' { print $1 }')

if [ "${a}" = "${b}" ];
then
echo "sha1sum checking OK for foo8!"
else
echo "sha1sum cheching Failed for foo8!"
fi

a=$(sha1sum tmp/foo.foo9 | awk '{ print $1 }')
b=$(sha1sum etc/B.tar | awk ' { print $1 }')

if [ "${a}" = "${b}" ];
then
echo "sha1sum checking OK for foo9!"
else
echo "sha1sum cheching Failed for foo9!"
fi

a=$(sha1sum tmp/foo.foo10 | awk '{ print $1 }')
b=$(sha1sum etc/B.tar | awk ' { print $1 }')

if [ "${a}" = "${b}" ];
then
echo "sha1sum checking OK for foo10!"
else
echo "sha1sum cheching Failed for foo10!"
fi

a=$(sha1sum tmp/foo.foo11 | awk '{ print $1 }')
b=$(sha1sum etc/B.tar | awk ' { print $1 }')

if [ "${a}" = "${b}" ];
then
echo "sha1sum checking OK for foo11!"
else
echo "sha1sum cheching Failed for foo11!"
fi

exit 0



