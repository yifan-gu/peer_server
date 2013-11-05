Project 2: Congestion Control with Bittorrent
======

In this project we implement a BitTorrent-like ﬁle transfer application.
The application will run on top of UDP, and we implement a reliability and congestion control protocol (similar to TCP) for the application.
The application will be able to simultaneously download different parts, called “chunks,” of a ﬁle from different servers.
The application will respond to chunk downloading request and upload data to requesting peer.


TOC
------

* ./readme.txt: this file
* ./src/: the source file folder
* ./src/obj: obj build file folder
* ./src/include: source header file folder
* ./src/test: unit testing file folder
* ./test/: the test bins
* ./vulnerabilities.txt: the vulnerabilities description
* ./Makefile: the Makefile
* ./etc/: the folder contains non source-code files
* ./tests.txt: the test case description


Structure of Modules
------

### Peer Server
This module give skeleton functions of starting, running servers, handling timeout.
It also provides some helper functions for peer server object.

### Peer Object and Peer List
Peer Object provides attributes needed to handle peer related work.
Because *peer.c* is taken by starter code, we put peer object code in *peerlist.c* .
Similarly, peer list provides attributes needed to handle peer list logistics.


### Chunk Object and Chunk list
This module provides chunk list so that we can parse the chunk file logic into easily accessible code.

### Raw packet handling

### Downloading: TCP Receive

### Uploading: TCP Send

### Utilities


Algorithm
------

### How we handle timeout

We handle timeout of following cases:

1. WHOHAS/GET timeout
If it's long enough after last time we send WHOHAS packet and we are still not downloading anything while we should,
we will resend WHOHAS packet.

2. DATA/ACK timeout
We record each DATA packet's timestamp to ensure better precision on timeout.
If the first packet inside current window is timeout or we receive duplicate ACK, we will do the following:
cut half threshold, do slow start on first packet in window.

We take all ACK packeks the same to update timestamp.

For both DATA and ACK, we have maximum timeout number.
If it's exceeded (timeout happens too many times), we will consider the connection is failing and closed.

### Update RTT



### Sending Window


### Receiving Window
