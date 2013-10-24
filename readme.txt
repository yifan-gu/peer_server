Project 2

a relieble p2p client.

This program is based on "select" structure for nonblocking I/O.
In each iteration during the main loop, the code will do:

1, receive packets, and response them
2, check if some connection is timeout
3, send data packets if appliable.

TOC
./readme.txt this file
./src/ the source file folder
./test/ the test bins
./vulnerabilities.txt the vulnerabilities description
./Makefile the Makefile
./etc/ the folder contains non-source data
./tests.txt the test case description


Updated Oct, 24:
Have implemented a simple "tcp", with congestion controll, but more tests are
needed!

Updated before Oct, 24:
Have implemented the packeting part, including encoding and decoding packets.
Have implemented the main control flow.

