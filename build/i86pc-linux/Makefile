#
#  Makefile for the Connection Server
#

RANLIB = ranlib

BINDIR := /p/xinu/bin
ETCDIR := /p/xinu/etc/contools

TOP = ../..

INCDIRS := $(TOP)/hdr $(INCDIRS)
SRCDIRS := $(TOP)/lib $(TOP)/programs $(TOP)/cserver $(SRCDIRS)

INCOPTS := $(patsubst %,-I%,$(INCDIRS))

CPPFLAGS := $(DEFINES) $(INCOPTS) $(CPPFLAGS)
CFLAGS := -g $(CFLAGS)
LDFLAGS := $(LDFLAGS)

VPATH := $(SRCDIRS)

LIB := lib.a

PROGRAMS :=  $(basename $(notdir $(wildcard $(TOP)/programs/*.c)))

all: $(PROGRAMS) cserver

$(PROGRAMS): % : %.o $(LIB)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIB)

LIBSOURCES := $(notdir $(wildcard $(TOP)/lib/*.c))

LIBOBJECTS := $(addsuffix .o, $(basename $(LIBSOURCES)))

$(LIB): $(LIBOBJECTS)

SERVERSOURCES :=  $(notdir $(wildcard $(TOP)/cserver/*.c)) scanner.c

SERVEROBJECTS := $(addsuffix .o, $(basename $(SERVERSOURCES)))

cserver: $(LIB) $(SERVEROBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(SERVEROBJECTS) $(LIB) -ll

scanner.c: $(TOP)/cserver/scanner.l
	$(LEX) -t $(TOP)/cserver/scanner.l > scanner.c

scanner.o: scanner.c

#
# Rules for cleaning up
#
clean:
	rm -f *.o *.a *.d *.map *.c
	rm -f *.out core

clobber: clean
	rm -f $(PROGRAMS) cserver

EPROGS1 := $(filter-out cs-console cs-break cs-status, $(PROGRAMS)) cserver 
EPROGS2 := $(filter-out cs-status RCS, $(notdir $(wildcard $(TOP)/scripts/*)))

BINPROGS := $(addprefix $(BINDIR)/, cs-console cs-break cs-status)
ETCPROGS := $(addprefix $(ETCDIR)/, $(EPROGS1) $(EPROGS2) cs-status)

$(BINDIR)/%: %
	@echo install $@
	@mkdir -p $(BINDIR)
	@rm -f $@
	@cp $^ $@
	@$(STRIP) $@
	@chmod 555 $@

$(ETCDIR)/%: %
	@echo install $@
	@mkdir -p $(ETCDIR)
	@rm -f $@
	@cp $^ $@
	@$(STRIP) $@
	@chmod 555 $@

$(BINDIR)/cs-status: ../../scripts/cs-status
	@echo install $@
	@mkdir -p $(BINDIR)
	@rm -f $@
	@cp $^ $@
	@chmod 555 $@

$(ETCDIR)/cs-status: cs-status
	@echo install $@
	@mkdir -p $(ETCDIR)
	@rm -f $@
	@cp $^ $@
	@$(STRIP) $@
	@chmod 555 $@

$(ETCDIR)/%: ../../scripts/%
	@echo install $@
	@mkdir -p $(ETCDIR)
	@rm -f $@
	@cp $^ $@
	@chmod 555 $@

install: all
	cp cs-console $(BINDIR)
	cp cs-status  $(BINDIR)
	chmod 0555 $(BINDIR)/cs-*


#
# Rule for building each of the libraries
#
%.a:
	$(AR) cr $@ $?
	$(RANLIB) $(RANLIBFLAGS) $@

depend:
	maketd $(CPPFLAGS) $(TOP)/*/*.c

