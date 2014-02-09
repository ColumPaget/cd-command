CC = gcc
VERSION = 0.0.1
CFLAGS = -g -O2
LIBS = -lz 
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
DESTDIR=
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_LIBZ=1
OBJ=cddb.o

all: $(OBJ) main.c
	@cd libUseful-2.0; $(MAKE)
	$(CC) $(FLAGS) -o cd-command $(LIBS) $(OBJ) main.c libUseful-2.0/libUseful-2.0.a 

cddb.o: cddb.c cddb.h
	$(CC) -c cddb.c

clean:
	@rm -f cd-command *.o libUseful-2.0/*.o libUseful-2.0/*.a libUseful-2.0/*.so

install:
	$(INSTALL) cd-command $(bindir)
