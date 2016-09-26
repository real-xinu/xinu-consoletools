#!/usr/bin/perl 
use lib '$ENV{HOME}/xinu-consoletools/scripts/perl';

use strict;

use File::Basename;
use XINUBackend;
use Data::Dumper;

my $backend_type = "quark";

my $script_name = basename($0);
my @backends = ();

my $usage = "Usage: $script_name xbootpath xinupath [backendname]
Updates the xboot image on the $backend_type backends
  xbootpath - Path to the xboot image
  xinupath  - Path to the xinu image to restore after loading xboot
  backendname (optional) - A specific backend to send files
";

my $xboot_file = shift or die $usage;
die "$xboot_file not found.\n" unless(-e $xboot_file);

my $xinu_file = shift or die $usage;
die "$xinu_file not found.\n" unless(-e $xinu_file);

my $specific_backend;
$specific_backend = shift or $specific_backend = "";

if($specific_backend ne "") {
	push @backends, $specific_backend;
} else {
	for(split/\n/, `/p/xinu/bin/cs-status -b -c $backend_type`) {
		if(/^(\w+)\s*$backend_type\s*user=\s*(\w+)\s*time/) {
			print "$1 is allocated to $2, skipping...\n";
		} elsif(/^(\w+)\s*$backend_type\s*user=\s+time/) {
			push @backends, $1;
		}
	}
}

for(@backends) {
	my $backend = new XINUBackend($backend_type, $ENV{'HOME'}, $_);
	$backend->send_files_to_linux(
		[$xboot_file],
		["/media/mmcblk0p1/xboot"],
		$xinu_file,
		"xinuserver.cs.purdue.edu"
	);
}
