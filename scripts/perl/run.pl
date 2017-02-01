#!/usr/bin/perl

use lib $ENV{HOME} . '/xinu-consoletools/scripts/perl';

use strict;

use XINUBackend;
use XINUTestCase;
use File::Basename;

$| = 1;

my $submit_name = "lab1";

my $output_timeout = 10;
my $script_name = basename($0);

my $user_home = $ENV{'HOME'};
my $course_home = "/homes/cs503";
#my $submit_path = "$course_home/submit/$submit_name";
my $submit_path = "$user_home/submit/$submit_name";
#my $run_path = "$course_home/grading/$submit_name\_grading";
my $run_path = "$user_home/cs503/$submit_name\_grading/compile";

# Build Image Names
my $xinu_images = [ map{ "xinu$_"; } (0 .. 7) ];
	
#run_student("$run_path/solution", "$run_path/solution.run_out", $xinu_images);

while(<$submit_path/*>) {
	my $submit_file = $_;
	my $uid = $submit_file =~ /(.*)\.Z$/ ? basename($1) : basename($submit_file);
	run_student("$run_path/$uid", "$run_path/$uid.run_out");
}

sub run_student {
	my $xinu_dir = shift;
	my $log_file = shift;

	if(-e $log_file) {
		print "$log_file: output file already exists skipping\n";
		return;
	}
	print "Running test cases for $xinu_dir\n";

	my $runner = new XINUBackend("quark", $xinu_dir);

	for(@{$xinu_images}) {
		$runner->add_xinu_image_to_run($_);
	}
	$runner->set_timeout($output_timeout);
	$runner->run_images($log_file);
}
