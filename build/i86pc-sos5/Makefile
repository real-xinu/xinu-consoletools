#
#  Makefile for the distributed bed server and associated programs
#

DEFINES = -DSOLARIS

include ../Tools.mk

CC := /usr/local/gnu/gcc
LD := /usr/local/gnu/gcc
RANLIB := /bin/echo

LDFLAGS := $(LDFLAGS) -lsocket -lnsl

include ../Makefile.mk

# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT
# Dependencies generated at: Fri Aug 11 17:21:42 EST 1995

breakConnection.o: ../../hdr/cserver.h
breakConnection.o: ../../lib/breakConnection.c
connectUtils.o: ../../hdr/cserver.h
connectUtils.o: ../../hdr/timeout.h
connectUtils.o: ../../lib/connectUtils.c
connectto.o: ../../lib/connectto.c
cs-break.o: ../../hdr/cserver.h
cs-break.o: ../../programs/cs-break.c
cs-command.o: ../../hdr/cserver.h
cs-command.o: ../../hdr/timeout.h
cs-command.o: ../../programs/cs-command.c
cs-console.o: ../../hdr/cserver.h
cs-console.o: ../../programs/cs-console.c
cs-status.o: ../../hdr/cserver.h
cs-status.o: ../../programs/cs-status.c
cserver.o: ../../cserver/cserver.c
cserver.o: ../../hdr/cserver.h
cserver.o: ../../hdr/timeout.h
getuser.o: ../../hdr/cserver.h
getuser.o: ../../lib/getuser.c
log.o: ../../cserver/log.c
makeConnection.o: ../../hdr/cserver.h
makeConnection.o: ../../lib/makeConnection.c
netutils.o: ../../lib/netutils.c
obtainStatus.o: ../../hdr/cserver.h
obtainStatus.o: ../../hdr/timeout.h
obtainStatus.o: ../../lib/obtainStatus.c
support.o: ../../cserver/support.c
support.o: ../../hdr/cserver.h
support.o: ../../hdr/tokens.h
telnet-connect.o: ../../programs/telnet-connect.c
tty-connect.o: ../../programs/tty-connect.c
ttymode.o: ../../lib/ttymode.c
ttyutils.o: ../../lib/ttyutils.c
utils.o: ../../lib/utils.c
x10.o: ../../programs/x10.c
xmalloc.o: ../../lib/xmalloc.c

# DO NOT ADD ANYTHING HERE - WILL GO AWAY
