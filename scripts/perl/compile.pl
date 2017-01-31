#!/usr/bin/perl

use lib $ENV{HOME} . '/xinu-consoletools/scripts/perl';

use strict;

use POSIX ":sys_wait_h";
use XINUTestCase;
use XINUBuild;
use File::Basename;

$| = 1;

my $submit_name = "lab1";

my $max_procs = 5;
my $script_name = basename($0);

my $user_home = $ENV{'HOME'};
my $course_home = "/homes/cs503";
my $submit_path = "$user_home/submit/$submit_name";
#my $compile_path = "$course_home/grading/$submit_name\_grading";
my $compile_path = "$user_home/cs503/$submit_name\_grading/compile";
#my $testcase_path = "$course_home/grading/$submit_name\_grading/testcases";
my $testcase_path = "$user_home/cs503/$submit_name\_grading/testcases";

# Array of test case files
#   An empty row means to run the submission as is (without modifications)
#   A row contains entries of files to replace in a 3-tuple
#	[ SRC_FILE, TGT_FILE, BACKUP_FILE ]
#		SRC_FILE the file to use for the testcase
#		TGT_FILE the file in the Xinu submission it should replace
#		BACKUP_FILE - the location for where to backup the original file
#	This script automatically prepends:
#		$testcase_path to the front of the SRC_FILE
#		The extracted submission XINU directory to the TGT_FILE and BACKUP_FILE
my @testcase_files = (
	[],
	[["main.c", "system/main.c"], ["prod_cons_ts0.h", "include/prod_cons.h"]],
	[["main.c", "system/main.c"], ["prod_cons_ts1.h", "include/prod_cons.h"]],
	[["main.c", "system/main.c"], ["prod_cons_ts2.h", "include/prod_cons.h"]],
	[["main.c", "system/main.c"], ["prod_cons_ts3.h", "include/prod_cons.h"]],
	[["main.c", "system/main.c"], ["prod_cons_ts4.h", "include/prod_cons.h"]],
	[["main.c", "system/main.c"], ["prod_cons_ts5.h", "include/prod_cons.h"]],
	[["main.c", "system/main.c"], ["prod_cons_ts6.h", "include/prod_cons.h"]],
	[["main.c", "system/main.c"], ["prod_cons_ts7.h", "include/prod_cons.h"]]
);

my @pids = ();
my $pid_count = 0;
while(<$submit_path/*>) {
	my $submit_file = $_;
	my $pid = fork();
	if($pid) {
		unshift @pids, $pid;
		$pid_count = $pid_count + 1;
		if($pid_count >= $max_procs) {
			my $wait_pid = shift @pids;
			waitpid($wait_pid, 0);
			$pid_count = $pid_count - 1;
		}
	} elsif($pid == 0) {
		compile(extract($submit_file, $compile_path));
		exit;
	} else {
		die "ERROR: Could not fork a new process: $!\n";
	}
}
for(@pids) {
	waitpid($_, 0);
}

sub find_xinu_dir {
	my $source_dir = shift;

	my $xinu_dir = "";
	while(<$source_dir/*>) {
		if(-d $_) {
			$xinu_dir = $_;
		}
	}
	die "Unable to find XINU directory in $source_dir\n" unless $xinu_dir ne "";
	return $xinu_dir;
}

sub compile {
	my $compile_dir = shift;

	my $uid = basename($compile_dir);

	print "Building: $compile_dir\n";
	my $xinu_dir = find_xinu_dir($compile_dir);
	my $builder = new XINUBuild($xinu_dir, $compile_dir);
	
	for my $i (0 .. $#testcase_files) {
		my $tc = new XINUTestCase("xinu$i");
		for(@{$testcase_files[$i]}) {
			my $tc_src = $_->[0];
			my $tc_tgt = $_->[1];
			my $tc_bkp = $_->[2];
			$tc->add_file("$testcase_path/$tc_src", $tc_tgt, $tc_bkp);
		}
		$builder->add_test_case($tc);
	}
	$builder->build_test_cases("$compile_dir/$uid.cmpout");
}

sub extract {
	my $submit_file = shift;
	my $target_dir = shift;

	print "Extracting: $submit_file\n";

	my $uid = basename($submit_file);
	my $extract_cmd = "cat $submit_file | tar -C $target_dir/$uid -xf -";
	if($submit_file =~ /(.*)\.Z$/) {
		$uid = basename($1);
		$extract_cmd = "zcat $submit_file | tar -C $target_dir/$uid -xf -";
	}
	my $compile_dir = "$target_dir/$uid";

	if(-e $compile_dir) {
		print "$compile_dir already exists\n";
		return;
	}
	system("mkdir -p $compile_dir") == 0 or die "Unable to make target compile directory: $?\n";
	system($extract_cmd) == 0 or die "Unable to extract student submission: $?\n";

	return $compile_dir;
}
