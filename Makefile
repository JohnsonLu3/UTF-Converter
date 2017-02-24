CC = gcc
SRC = src/
INCLUDE = include/
BIN = utfconverter
CFLAGS = -Wall -Werror -pedantic -Wextra 
REQ = $(SRC) 
BLDDIR = bin build
IDIR = include/
SDIR = src/
BIND = bin/
BUILD = build/
MKD = makeDir

all: $(MKD) utf utfconverter.o

makeDir:
	mkdir -p build
	mkdir -p bin

$(BIN): $(REQ)
	$(CC) $(CFLAGS) -I$(IDIR) $(SRC) -o $(BIND)utf

.PHONY: clean

utfconverter.o: 
	gcc $(CFLAGS) -I$(IDIR) $(SDIR)utfconverter.c -o $(BUILD)utfconverter.o -c $^

utf:
	gcc $(CFLAGS) -I$(IDIR) $(SDIR)utfconverter.c -o $(BIND)utf -lm

debug:
	mkdir -p build
	mkdir -p bin
	$(CC) $(CFLAGS) -g -I$(IDIR) -o $(BIND)utf $(SDIR)utfconverter.c -lm

clean:
	rm -f *.o $(BIN)
	rm -r $(BLDDIR)
