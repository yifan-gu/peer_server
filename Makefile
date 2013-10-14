IDIR =src/inc
ODIR=src/obj
SRCDIR=src

CC 		= gcc
CFLAGS=-I$(IDIR) -Wall -Werror -DDEBUG
LDFLAGS	= -lm

TESTDEFS	= -DTESTING			# comment this out to disable debugging code

_HEADERS = bt_parse.h  chunk.h  debug.h  debug-text.h  input_buffer.h  sha.h  spiffy.h
HEADERS = $(patsubst %,$(IDIR)/%,$(_HEADERS))

_OBJS = peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_MK_CHUNK_OBJS   = make_chunks.o chunk.o sha.o
MK_CHUNK_OBJS = $(patsubst %,$(ODIR)/%,$(_MK_CHUNK_OBJS))

TESTDEFS	= -DTESTING			# comment this out to disable debugging code

BINS = peer make-chunks

# Explit build and testing targets

all: ${BINS} ${TESTBINS}

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

