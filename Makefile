IDIR =src/include
ODIR=src/obj
SRCDIR=src
TESTDIR=src/test
TESTBINDIR=test

CC 		= gcc
CFLAGS=-I$(IDIR) -Wall -Werror -DDEBUG
LDFLAGS	= -lm

TESTDEFS	= -DTESTING			# comment this out to disable debugging code

_HEADERS = bt_parse.h  chunk.h  debug.h  debug-text.h  input_buffer.h  sha.h  spiffy.h \
		   logger.h  packet.h peer_server.h peerlist.h download.h upload.h linkedlist.h \
		   parse_packet.h peer.h send_helper.h
HEADERS = $(patsubst %,$(IDIR)/%,$(_HEADERS))

_OBJS = peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o \
		logger.o peer_server.o peerlist.o download.o upload.o linkedlist.o packet.o \
		parse_packet.o send_helper.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_MK_CHUNK_OBJS   = make_chunks.o chunk.o sha.o
MK_CHUNK_OBJS = $(patsubst %,$(ODIR)/%,$(_MK_CHUNK_OBJS))

_TEST_OBJS   = logger.o peer_server.o peerlist.o chunk.o sha.o packet.o linkedlist.o bt_parse.o debug.o
TEST_OBJS = $(patsubst %,$(ODIR)/%,$(_TEST_OBJS))

BINS = peer make-chunks

TESTBINS = test_packet test_send_message test_recv_message

# Explit build and testing targets
all: ${BINS}

test: ${TESTBINS}

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
		$(ODIR)/* $(SRCDIR)/*~ $(IDIR)/*~ $(SRCDIR)/*.orig $(IDIR)/*.orig $(TESTBINDIR)/* $(TESTDIR)/*.o

test_packet: $(TEST_OBJS) $(TESTDIR)/test_packet.o
	$(CC) -DTESTING_PACKET $(TEST_OBJS) $(TESTDIR)/test_packet.o -o $(TESTBINDIR)/test_packet $(LDFLAGS)

test_send_message: $(TEST_OBJS) $(TESTDIR)/test_send_message.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_send_message.o -o $(TESTBINDIR)/test_send_message $(LDFLAGS)

test_recv_message: $(TEST_OBJS) $(TESTDIR)/test_recv_message.o
	$(CC) -DDEBUG $(TEST_OBJS) $(TESTDIR)/test_recv_message.o -o $(TESTBINDIR)/test_recv_message $(LDFLAGS)
