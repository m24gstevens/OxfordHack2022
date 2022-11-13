CC=gcc
CFLAGS = -Ofast -fomit-frame-pointer
DEPS = oxchess.h bitboard.h board.h move.h movegen.h uci.h evaluate.h search.h order.h tt.h nnueval.h
OBJ = oxchess.o bitboard.o board.o move.o movegen.o uci.o evaluate.o search.o order.o tt.o nnueval.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

OxChessBot: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse -L. -l:libnnueprobe.so

clean:
	rm -f *.o
	rm -f OxChessBot