TARGET = ss_daemon

IDIR = ../include
LDIR = ../lib
ODIR = obj

CC = gcc
CFLAGS = -Wall -g -I$(IDIR)

LIBS = -lm

_DEPS = main.h init.h utils.h dir_monitor.h config.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o init.o utils.o dir_monitor.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
	