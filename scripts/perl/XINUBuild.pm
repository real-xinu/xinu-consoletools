package XINUBuild;

use strict;

use Data::Dumper;
use File::Copy;
use File::Path;
use Cwd;
use IPC::Open3;

use constant VERBOSE => 1;

if(VERBOSE) {
	$| = 1;
}

sub new {
	my $class = shift;
	my $xinu_directory = shift;
	my $target_directory = shift;

	my $self = { 
		'xinu_directory' => $xinu_directory,
		'target_directory' => $target_directory
	};

	bless $self, $class;
	return $self;
}

sub add_test_case {
	my $self = shift;
	my $test_case = shift;

	if(defined($self->{'test_cases'}->{$test_case->image_name})) {
		return;
	}
	$self->{'test_cases'}->{$test_case->image_name} = $test_case;
}

sub get_test_case_images {
	my $self = shift;
	return keys %{$self->{'test_cases'}};
}

sub build_test_cases {
	my $self = shift;
	my $results_file = shift;

	my $results_handle;

	# Set the results path
	#   Append to target directory unless user passed in full path (begin with '/')
	if(defined($results_file)) {
		my $results_path = $results_file;
		unless($results_file =~ /^\//) {
			$results_path = $self->{'target_directory'} . "/$results_path"
		}
		if(-e $results_path) {
			_munlink($results_path);
		}	
		open $results_handle, ">>", $results_path or die "Unable to open $results_path: $!\n";
		_print_msg("build_test_cases :: Results printing to $results_path\n");
	} else {
		$results_handle = *STDOUT;
		_print_msg("build_test_cases :: Results printing to STDOUT\n");
	}

	unless(-d $self->{'xinu_directory'} . "/compile") {
		die "Xinu compile directory ($self->{'xinu_directory'}/compile) not found\n";
	}

	my $xinu_directory = $self->{'xinu_directory'};
	my $target_image_dir = $self->{'target_directory'};

	for(sort keys %{$self->{'test_cases'}}) {
		my $image = $_;
		my $test_case = $self->{'test_cases'}->{$image};
		my $target_image_path = "$target_image_dir/$image";

		my $cwd = getcwd;

		_mchdir($self->{'xinu_directory'} . "/compile");

		if(defined($test_case->source_files) && defined($test_case->target_files)) {

			my @source_files = @{$test_case->source_files};
			my @target_files = @{$test_case->target_files};
			my @backup_files = @{$test_case->backup_files};

			for(0 .. $#source_files) {
				if(-e "$xinu_directory/" . $target_files[$_]) {
					if($backup_files[$_] ne "") {
						_mcopy("$xinu_directory/" . $target_files[$_], "../" . $backup_files[$_]);
					}
					_munlink("$xinu_directory/" . $target_files[$_]);
				}
				_mcopy($source_files[$_], "$xinu_directory/" . $target_files[$_]);
			}
		}

		if(-e $target_image_path) {
			_munlink($target_image_path);
		}

		print $results_handle "Building Test Case $image\n";
		_msystem_capture_print($results_handle, "make clean");
		_msystem_capture_print($results_handle, "make depclean");
		_msystem_capture_print($results_handle, "make defclean");
		_msystem_capture_print($results_handle, "make rebuild");
		_msystem_capture_print($results_handle, "make depend");
		_msystem_capture_print($results_handle, "make");

		if(-e "xinu.xbin") {
			_mmove("xinu.xbin", $target_image_path);
		} elsif(-e "xinu") {
			_mmove("xinu", $target_image_path);
		}
		if(-e "xinu.elf") {
			_mmove("xinu.elf", "$target_image_path.elf");
		}

		_mchdir($cwd);
	}

	if(defined($results_file)) {
		close $results_handle;
	}
}
	
sub _msystem_capture_print {
	my $handle = shift;
	my $command = shift;

	_print_msg("_msystem_capture_print :: Running command $command\n");

	my($wtr, $rdr);
	my $pid = open3($wtr, $rdr, $rdr, $command);

	while(<$rdr>) {
		print $handle $_;
	}

	close($wtr);
	close($rdr);

	waitpid($pid, 1);
}

sub _mchdir {
	my $directory = shift;
	_print_msg("_mchdir :: Changing to directory: $directory\n");
	chdir $directory or die "Unable to change to directory $directory: $!\n";
}

sub _mmkdir {
	my $directory = shift;
	_print_msg("_mmkdir :: Creating directory: $directory\n");
	mkdir $directory or die "Unable to make directory $directory: $!\n";
}

sub _mcopy {
	my ($src, $tgt) = @_;
	_print_msg("_mcopy :: Copying $src to $tgt\n");
	copy $src, $tgt or die "Unable to copy $src to $tgt: $!\n";
}

sub _mmove {
	my ($src, $tgt) = @_;
	_print_msg("_mmove :: Moving $src to $tgt\n");
	move $src, $tgt or die "Unable to move $src to $tgt: $!\n";
}

sub _munlink {
	my $file = shift;
	_print_msg("_munlink :: Unlinking $file\n");
	unlink $file or die "Unable to remove file $file: $!\n";
}

sub _msystem {
	system(@_) == 0 or die "Unable to run system @_: $?\n"
}

sub _print_msg {
	if(VERBOSE) {
		print @_;
	}
}

return 1;
