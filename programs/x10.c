#include <ctype.h>
#include <stdio.h>
#include <malloc.h>

unsigned char House[] = {
	0x60 /*A*/, 0xe0 /*B*/, 0x20 /*C*/, 0xa0 /*D*/,
	0x10 /*E*/, 0x90 /*F*/, 0x50 /*G*/, 0xd0 /*H*/,
	0x70 /*I*/, 0xf0 /*J*/, 0x30 /*K*/, 0xb0 /*L*/,
	0x00 /*M*/, 0x80 /*N*/, 0x40 /*O*/, 0xc0 /*P*/,
};

unsigned short UnitMasks[] = {
	0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
	0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
};

unsigned char
HouseCode( house )
char house;
{
	return( House[toupper(house) - 'A'] );
}

char
HouseName( housecode )
unsigned char housecode;
{
	int i;
	for( i = 0; i < 16; i++ ) {
		if( House[i] == (housecode&0xf0) ) {
			return( i + 'A' );
		}
	}
	return( 'A' );
}

short
UnitMask( unit )
int unit;
{
	return( UnitMasks[unit-1] );
}

void
Send( fd, message, len )
int fd;
unsigned char * message;
int len;
{
	int i;
	for( i = 0; i < len; i++ ) {
		if( write( fd, &message[i], 1 ) != 1 ) {
			fprintf( stderr, "x10: write failed\n" );
			exit( 1 );
		}
	}
	free( message );
}

void
SendSync( fd )
int fd;
{
	int i;
	unsigned char ff = 0xff;
	for( i = 0; i < 16; i++ ) {
		if( write( fd, &ff, 1 ) != 1 ) {
			fprintf( stderr, "x10: write failed\n" );
			exit( 1 );
		}
	}
}


unsigned char *
DirectMessage( house, unit, flag )
char house;
int unit;
int flag;
{
	unsigned short um;
	unsigned char * message = (unsigned char *) malloc( 6 );
	message[0] = 0x01;
	if( flag ) {
		message[1] = 0x02;
	}
	else {
		message[1] = 0x03;
	}
	message[2] = HouseCode( house );
	um = UnitMask( unit );
	message[3] = (um >> 8);
	message[4] = (um & 0xff);

	message[5] = (message[1] + message[2] + message[3]
		+ message[4]) & 0xff;
	return( message );
}

unsigned char *
OnMessage( house, unit )
char house;
int unit;
{
	return( DirectMessage( house, unit, 1 ) );
}

unsigned char *
OffMessage( house, unit )
char house;
int unit;
{
	return( DirectMessage( house, unit, 0 ) );
}

void
TurnOn( fd, house, unit )
int fd;
char house;
int unit;
{
	unsigned char * msg;

	fprintf( stderr, "turning on %c:%d\n", house, unit );
	
	SendSync( fd );
	msg = OnMessage( house, unit );
	Send( fd, msg, 6 );
}

void
TurnOff( fd, house, unit )
int fd;
char house;
int unit;
{
	unsigned char * msg;

	fprintf( stderr, "turning off %c:%d\n", house, unit );
	
	SendSync( fd );
	msg = OffMessage( house, unit );
	Send( fd, msg, 6 );
}

printusage( sb )
     char * sb;
{
	fprintf( stderr, "usage: %s [0|1|p] house unit [dev]\n", sb );
	exit( 1 );
}

main( argc, argv )
int argc;
char * argv[];
{
	char house;
	int unit;
	int len;
	int fd = 1;

	if( argc < 4 ) {
		printusage( argv[ 0 ] );
	}

	house = argv[2][0];
	unit = atoi( argv[3] );
	if( argc == 5 ) {
		fd = OpenTTYLine( argv[4], "600" );
	}

	if( strcmp( argv[ 1 ], "0" ) == 0 ) {
		TurnOff( fd, house, unit );
	}
	else if( strcmp( argv[ 1 ], "1" ) == 0 ) {
		TurnOn( fd, house, unit );
	}
	else if( strcmp( argv[ 1 ], "p" ) == 0 ) {
		TurnOff( fd, house, unit );
		sleep( 5 );
		TurnOn( fd, house, unit );
	}
	else {
		printusage( argv[ 0 ] );
	}
}

