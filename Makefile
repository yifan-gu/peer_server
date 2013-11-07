IDIR =src/include
ODIR=src/obj
SRCDIR=src
TESTDIR=src/test
TESTBINDIR=test/bin
MULTITEST=test/multitest

CC 		= gcc
CFLAGS=-I$(IDIR) -Wall -Werror -DDEBUG
LDFLAGS	= -lm

TESTDEFS	= -DTESTING			# comment this out to disable debugging code

_HEADERS = bt_parse.h  chunk.h  debug.h  debug-text.h  input_buffer.h  sha.h  spiffy.h \
		   logger.h  packet.h peer_server.h peerlist.h download.h upload.h linkedlist.h \
		   parse_packet.h peer.h send_helper.h util.h chunklist.h recv_win.h
HEADERS = $(patsubst %,$(IDIR)/%,$(_HEADERS))

_OBJS = peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o \
		logger.o peer_server.o peerlist.o download.o upload.o linkedlist.o packet.o \
		parse_packet.o send_helper.o util.o chunklist.o recv_win.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_MK_CHUNK_OBJS   = make_chunks.o chunk.o sha.o
MK_CHUNK_OBJS = $(patsubst %,$(ODIR)/%,$(_MK_CHUNK_OBJS))

_TEST_OBJS   = logger.o peer_server.o peerlist.o chunk.o sha.o packet.o linkedlist.o send_helper.o chunklist.o recv_win.o \
		bt_parse.o debug.o download.o spiffy.o util.o upload.o
TEST_OBJS = $(patsubst %,$(ODIR)/%,$(_TEST_OBJS))

BINS = peer make-chunks

TESTBINS = test_send_message test_recv_message test_send_corrupt test_upload test_receiver \
	test_download test_sender #test_packet

# Explit build and testing targets
all: ${BINS}

test: ${TESTBINS} peer
	cp peer ${MULTITEST}

peer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

make-chunks: $(MK_CHUNK_OBJS)
	$(CC) $(CFLAGS) $(MK_CHUNK_OBJS) -o $@ $(LDFLAGS)

$(ODIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(TESTDEFS)

$(TESTDIR)/%.o: $(TESTDIR)/%.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(TESTDEFS)

.PHONY: clean
clean:
	@rm -f $(BINS) \
		$(ODIR)/* $(SRCDIR)/*~ $(IDIR)/*~ $(SRCDIR)/*.orig $(IDIR)/*.orig $(TESTBINDIR)/* $(TESTDIR)/*.o \
		output.dat verytemp problem2-peer.txt $(MULTITEST)/tmp/*

#test_packet: $(TEST_OBJS) $(TESTDIR)/test_packet.o
#	$(CC) -DTESTING_PACKET $(TEST_OBJS) $(TESTDIR)/test_packet.o -o $(TESTBINDIR)/test_packet $(LDFLAGS)

test_send_message: $(TEST_OBJS) $(TESTDIR)/test_send_message.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_send_message.o -o $(TESTBINDIR)/test_send_message $(LDFLAGS)

test_recv_message: $(TEST_OBJS) $(TESTDIR)/test_recv_message.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_recv_message.o -o $(TESTBINDIR)/test_recv_message $(LDFLAGS)

test_send_corrupt: $(TEST_OBJS) $(TESTDIR)/test_send_corrupt.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_send_corrupt.o -o $(TESTBINDIR)/test_send_corrupt $(LDFLAGS)

test_upload: $(TEST_OBJS) $(TESTDIR)/test_upload.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_upload.o -o $(TESTBINDIR)/test_upload $(LDFLAGS)

test_receiver: $(TEST_OBJS) $(TESTDIR)/test_receiver.o $(ODIR)/input_buffer.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_receiver.o $(ODIR)/input_buffer.o -o $(TESTBINDIR)/test_receiver $(LDFLAGS)

test_download: $(TEST_OBJS) $(TESTDIR)/test_download.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_download.o -o $(TESTBINDIR)/test_download $(LDFLAGS)

test_sender: $(TEST_OBJS) $(TESTDIR)/test_sender.o $(ODIR)/input_buffer.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_sender.o $(ODIR)/input_buffer.o  -o $(TESTBINDIR)/test_sender $(LDFLAGS)
