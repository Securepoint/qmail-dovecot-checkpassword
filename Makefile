DESTDIR =
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

INSTALL = install
CFLAGS  ?= -Os -W -Wall

CC=gcc
BUILDDIR = .
OBJ = $(BUILDDIR)/checkpasswd.o
OBJ += $(BUILDDIR)/base64.o

BIN = $(BUILDDIR)/checkpasswd-dovecot

all: $(BUILDDIR) $(BIN)

install: all
	$(INSTALL) -d -m 755 $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 755 $(BIN) $(DESTDIR)$(BINDIR)
	
clean: 
	$(VERBOSE)rm -f $(OBJ) $(BIN)

INDENT = indent -nbad -nbap -nbbb -nbc -bl -c33 -ncdb -ce -cli1 -d0 -di0 -ndj -nfc1 -i2 -l78 -nlp -npcs -npsl -sc -sob -nut -saf -sai -saw -br
INDENTSRC   = $(wildcard *.[ch])
indent: $(INDENTSRC)
	for each in $(INDENTSRC); do $(INDENT) $$each; rm *~; done


$(BUILDDIR):
	$(VERBOSE)mkdir -p $(BUILDDIR)


$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)


