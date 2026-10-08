CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
SRC     = main.c util.c lexical_analyzer.c symbol_table.c assembler.c \
          macro_processor.c linker_loader.c recursive_descent_parser.c \
          quadruple.c optimizer.c

toolkit: $(SRC) toolkit.h
	$(CC) $(CFLAGS) -o toolkit $(SRC)

clean:
	rm -f toolkit
