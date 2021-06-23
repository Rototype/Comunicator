all:Comunicator

IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR) -g

ODIR=obj
LDIR =../lib

LIBS=-lpthread -lsqlite3 -lm 

_DEPS = usesys.h global.h extern.h define.h struttur.h protocol.h appglob.h appext.h appdef.h areadati.h appstrutt.h ws.h base64.h sha1.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = Comunicator.o protocol.o logica.o prSPI.o prSPISlave.o prWebSocket.o simulaHWC.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Comunicator: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

