CC=gcc
CFLAGS = -g
DEPS = oxchess.h bitboard.h board.h move.h movegen.h uci.h evaluate.h search.h order.h tt.h
OBJ = oxchess.o bitboard.o board.o move.o movegen.o uci.o evaluate.o search.o order.o tt.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

OxChessBot: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f OxChessBot