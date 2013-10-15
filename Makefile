IDIR =src/include
ODIR=src/obj
SRCDIR=src
TESTDIR=src/test

CC 		= gcc
CFLAGS=-I$(IDIR) -Wall -Werror -DDEBUG
LDFLAGS	= -lm

TESTDEFS	= -DTESTING			# comment this out to disable debugging code

_HEADERS = bt_parse.h  chunk.h  debug.h  debug-text.h  input_buffer.h  sha.h  spiffy.h \
		   logger.h  packet.h peer_server.h peerlist.h download.h upload.h linkedlist.h
HEADERS = $(patsubst %,$(IDIR)/%,$(_HEADERS))

_OBJS = peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o \
		logger.o peer_server.o peerlist.o download.o upload.o linkedlist.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_MK_CHUNK_OBJS   = make_chunks.o chunk.o sha.o
MK_CHUNK_OBJS = $(patsubst %,$(ODIR)/%,$(_MK_CHUNK_OBJS))

BINS = peer make-chunks

TESTBINS = test_packet

# Explit build and testing targets
all: ${BINS} ${TESTBINS}

test: ${TESTBINS}

peer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

make-chunks: $(MK_CHUNK_OBJS)
	$(CC) $(CFLAGS) $(MK_CHUNK_OBJS) -o $@ $(LDFLAGS)

$(ODIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(TESTDEFS)

.PHONY: clean
clean:
	@rm -f $(BINS) \
		$(ODIR)/* $(SRCDIR)/*~ $(IDIR)/*~ $(SRCDIR)/*.orig $(IDIR)/*.orig

test_packet:
	$(CC) -I$(IDIR) $(TESTDIR)/test_packet.c $(SRCDIR)/chunk.c $(SRCDIR)/sha.c -o test/test_packet
