#
#############################################################################
#
#        Copyright 1989, Xylogics, Inc.  ALL RIGHTS RESERVED.
#
# ALL RIGHTS RESERVED. Licensed Material - Property of Xylogics, Inc.
# This software is made available solely pursuant to the terms of a
# software license agreement which governs its use.
# Unauthorized duplication, distribution or sale are strictly prohibited.
#
# Makefile description:
#	Makes libannex library routines
#
# Original Author: Roger Parker	Created on: 1/5/87
#
# Revision Control Information:
#
# $Header: /home/jalembke/cvs/ConsoleTools/reboot/libannex/Makefile,v 1.1 2002/01/14 19:24:16 brylow Exp $
#
# This file created by RCS from
# $Source: /home/jalembke/cvs/ConsoleTools/reboot/libannex/Makefile,v $
#
# This file is currently under revision by:
# $Locker:  $
#
#############################################################################
#
#  DATE:	$Date: 2002/01/14 19:24:16 $
#  REVISION:	$Revision: 1.1 $
#

LIBANX_OT = ${LIBANX_O} .

GFLG =

EFILES = libannex.a

all:
	make -f ../make.config -f Makefile ${EFILES}

remake:
	make clean
	make all

clean:
	make -f ../make.config -f Makefile cleanup

cleanup:
	${RM} -f ${LIBANX_O} ${EFILES}

.c.o:
	${CC} ${CFLAGS} -c $<

${LIBANX_OT}:	../make.config

libannex.a:	${LIBANX_O}
	${AR} vur libannex.a ${LIBANX_O}
	${RANLIB} libannex.a

install:	all

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

bcopy.o: bcopy.c ../inc/config.h
dup2.o: dup2.c ../inc/config.h
erpc_subr.o: erpc_subr.c ../inc/config.h ../inc/courier/courier.h
erpc_subr.o: ../inc/erpc/erpc.h
srpc.o: srpc.c ../inc/config.h srpc.h ../inc/courier/courier.h
hostname.o: hostname.c ../inc/config.h
in_cksum.o: in_cksum.c ../inc/config.h ../inc/slip/slip_user.h
inet.o: inet.c ../inc/config.h
ip_layer.o: ip_layer.c ../inc/config.h ../inc/slip/slip_user.h
ip_layer.o: ../inc/slip/slip_system.h
sendrecv.o: sendrecv.c ../inc/config.h
slip_layer.o: slip_layer.c ../inc/config.h ../inc/slip/slip_user.h
slip_layer.o: ../inc/slip/slip_system.h ../inc/slip/BSDslip.h
socket_layer.o: socket_layer.c ../inc/config.h ../inc/slip/slip_user.h
udp_layer.o: udp_layer.c ../inc/config.h ../inc/slip/slip_user.h
udp_layer.o: ../inc/slip/slip_system.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
