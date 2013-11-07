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
* ./test/multitest/: some concurrent testing
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

### Network packet Encoding and Decoding
This module provides interfaces to encode/decode packets, as well as some helpful functions to get specific fields in the packet.

### Packet Type Handling
We build a sophisticated state machine to handle different types of packets
and provide the logic/workflow to process them.

### Download
This module provides an implementation of the receiving part of tcp. We use a circular queue as the buffer the receive the data.
When finishe downloading one chunk, this module will save the chunk to the output file.
This module also handle timeouts.

### Uploading: TCP Send
This module provides an implementation of the sending part of tcp. It implements Slow Start, Congestion Avoidance and Fast Retransmission.
Also, it handles dup acks and timeouts.
This module computes RTT and RTO dynamically, we adopted a backoff timeouts with upbound in Fast Retransmission as decribed in Karn's Algorithm.

### Utilities
This module provides some helpful functions, like computing the timestamp, computing the timeouts, etc.

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

Generally, we use every ACK to get sample RTT, and update the RTT by RTT = 0.875 * RTT + 0.125 * sample RTT,
except for Fast Retransmission. We do not use ACK of the retransmitted packets to compute RTT, so we can avoid ambiguous ACK.

### Sending Window

1. Slow Start
In Slow Start, the sending window size grows one unit every time receiving an valid ACK until reaching the threshold,
then it changes state to Congestion Avoidance. We also slides the windows forward if necessary.

If it loss a packet in this phase(dup ACK or timeouts), it will change state to FastRetransmission.

2. Congestion Avoidance
In Congestion Avoidance, the sending window grows one unit every RTT period pasts if no dup ACK or timeouts happens. Otherwise it changes state to FastRetransmission

3. FastRetransmission
In Fast retransmission, the window size starts at one, it will wait the ACK for the first retransmitted packet, and then change to Slow Start, if the ACK for the first
retransmitted packet timeouts, it will retransmit the packet again.

### Receiving Window
The receive window is of size 1024, which means we can store at most 1024 packets before sliding the ack pointer.
The receive window is implemented using rotational bitmap array.
