#
##############################################################################
#
#        Copyright 1989, Xylogics, Inc.  ALL RIGHTS RESERVED.
#
# ALL RIGHTS RESERVED. Licensed Material - Property of Xylogics, Inc.
# This software is made available solely pursuant to the terms of a
# software license agreement which governs its use.
# Unauthorized duplication, distribution or sale are strictly prohibited.
#
# Makefile description:
#
#	Builds be-reset
#
# Revision Control Information:
#
# $Header: /home/jalembke/cvs/ConsoleTools/reboot/be-reset/Makefile,v 1.1 2002/01/14 19:24:00 brylow Exp $
#
# This file created by RCS from:
# $Source: /home/jalembke/cvs/ConsoleTools/reboot/be-reset/Makefile,v $
#
# This file is currently under revision by:
# $Locker:  $
#
##############################################################################
#
# DATE:		$Date: 2002/01/14 19:24:00 $
# REVISION:	$Revision: 1.1 $
#

CFILES = be-reset.c
OFILES = be-reset.o

all:
	make -f ../make.config -f Makefile be-reset

clean:
	make -f ../make.config -f Makefile cleanup

install:	all
	make -f ../make.config -f Makefile installit

remake:
	make clean
	make all

.c.o:
	${CC} ${CFLAGS} -c $<

cleanup:
	${RM} -f ${OFILES} be-reset

be-reset:		${OFILES} ../libannex/libannex.a
	${CC} ${CFLAGS} -o be-reset ${OFILES} \
		../libannex/libannex.a ${LIBFLAG}

${OFILES}:	../make.config

../libannex/libannex.a:	../make.config
	cd ../libannex; make

installit:
	$(INSTALL) $(INSTALL_DIR) be-reset

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

be-reset.o: be-reset.c ../inc/config.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
