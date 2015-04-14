CC=gcc
CFLAGS=-c -Wall -Iinc
SOURCES=hl-atcmd.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=hl-atcmd

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
