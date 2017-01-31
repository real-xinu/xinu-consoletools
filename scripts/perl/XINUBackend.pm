package XINUBackend;

use strict;

use Expect;
use constant VERBOSE => 1;
use constant LINUX_PROMPT => "root\@clanton:~# ";

if(VERBOSE) {
	$| = 1;
}

my %platform_info = (
		'quark' => \&_handle_quark_run
	);

sub new {
	my $class = shift;
	my $backend_type = shift;
	my $target_directory = shift;
	my $backend_name = shift;

	# Make sure backend type is supported
	my $found = 0;
	for(keys %platform_info) {
		if($backend_type eq $_) {
			$found = 1;
			last;
		}
	}
	if($found == 0) {
		die "Unsupported backend type: $backend_type\n";
	}

	my $self = { 
		'backend_type' => $backend_type,
		'default_timeout' => 10,
		'default_key_delay' => 0.15,
		'default_max_wait' => 60,
		'retries' => 3,
		'target_directory' => $target_directory,
		'backend_name' => $backend_name
	};

	bless $self, $class;
	return $self;
}

sub set_timeout {
	my $self = shift;
	my $timeout = shift;
	my $old_val = $self->{'timeout'};
	$self->{'timeout'} = $timeout;
	return $old_val;
}

sub set_max_wait {
	my $self = shift;
	my $max_wait = shift;
	my $old_val = $self->{'max_wait'};
	$self->{'max_wait'} = $max_wait;
	return $old_val;
}

sub add_xinu_image_to_run {
	my $self = shift;
	my $image_name = shift;

	if(defined($self->{'images'}->{$image_name})) {
		print "Image $image_name already in list\n";
		return;
	}
	$self->{'images'}->{$image_name} = $image_name;
}

sub send_files_to_linux {
	my $self = shift;
	my $local_files = shift;
	my $remote_files = shift;
	my $xinu_image_path = shift;
	my $server = shift;
	
	my $backend = $self->{'backend_name'};
	unless(defined($backend)) {
		_error_msg("Backend required\n");
		return;
	}

	for(@{$local_files}) {
		unless(-e $_) {
			_error_msg("File $_ found.\n\n");
			return;
		}
	}
	unless(-e $xinu_image_path) {
		_error_msg("NO image $xinu_image_path found.\n\n");
		return;
	}

	return unless(_boot_to_spi_linux($self));

	_print_msg("Starting networking\n");
	_send_linux_cmd($self, "/etc/init.d/networking start");

	for(0 .. scalar(@{$local_files})-1) {
		_send_file_to_linux($self, $local_files->[$_], $remote_files->[$_], $server);
	}

	_close_backend_connection($self);
	
	_print_msg("Downloading xinu image\n");
	my $dl_results = _download_xinu_image($backend, $xinu_image_path);
	if(defined($dl_results)) {
		_error_msg("Unable to download image to backend: $backend:\n");
		_error_msg($dl_results);
		return;
	}

	my $pc_results = _power_cycle_backend($backend);
	if(defined($pc_results)) {
		_error_msg("Unable to power cycle backend: $backend:\n");
		_error_msg($pc_results);
		return;
	}
}

sub _boot_to_spi_linux {
	my $self = shift;

	my $backend = $self->{'backend_name'};
	unless(defined($backend)) {
		_error_msg("Backend required\n");
		return undef;
	}
	
	my $kernel_cmd = "kernel --spi root=/dev/ram0 console=ttyS1,115200n8 earlycon=uart8250,mmio32";
	my $initrd_cmd = "initrd --spi";

	unless(defined(_open_backend_connection($self))) {
		_error_msg("Unable to open a connection to $backend\n");
		return undef;
	}

	my $pc_results = _power_cycle_backend($backend);
	if(defined($pc_results)) {
		_error_msg("Unable to power cycle backend: $backend:\n");
		_error_msg($pc_results);
		_close_backend_connection($self);
		return undef;
	}

	_print_msg("Waiting for GRUB Menu\n");
	return undef unless(_expect_close_on_error($self, 20, ["GNU GRUB"]));
	
	_print_msg("Waiting for GRUB prompt\n");
	$self->{'csconsole'}->send("c");
	return undef unless(_expect_close_on_error($self, ["grub>"]));

	_print_msg("Sending kernel command\n");
	$self->{'csconsole'}->send_slow($self->{'default_key_delay'}, "$kernel_cmd\n");
	_print_msg("Waiting for kernel command output\n");
	my $patidx = _expect_close_on_error($self, ["grub>", "Hit return to continue"]);
	while($patidx == 2) {
		$self->{'csconsole'}->send("\n");
		$patidx = _expect_close_on_error($self, ["grub>", "Hit return to continue"]);
	}
	return undef unless(defined($patidx));
	$self->{'csconsole'}->send_slow($self->{'default_key_delay'}, "$initrd_cmd\n");
	return undef unless(_expect_close_on_error($self, ["grub>"]));
	_print_msg("Booting to Linux\n");
	$self->{'csconsole'}->send_slow($self->{'default_key_delay'}, "boot\n");
	return undef unless(_expect_close_on_error($self, 30, ["clanton login:"]));
	$self->{'csconsole'}->send_slow($self->{'default_key_delay'}, "root\n");
	return undef unless(_expect_close_on_error($self, [LINUX_PROMPT]));

	return 1;
}

sub _send_linux_cmd {
	my $self = shift;
	my $command = shift;

	$self->{'csconsole'}->send_slow($self->{'default_key_delay'}, "$command\n");
	return undef unless(_expect_close_on_error($self, [LINUX_PROMPT]));

	my $cmd_output = $self->{'csconsole'}->before();
	
	my $out_idx = 0;
	for my $cmd_idx (0 .. length($command)) {
		while(substr($cmd_output, $out_idx, 1) =~ /[\n\r]/) { $out_idx++; }
		my $cmdchar = substr($command, $cmd_idx, 1);
		my $outchar = substr($cmd_output, $out_idx, 1);
		last unless($outchar eq $cmdchar);
		$out_idx++;
	}
	while(substr($cmd_output, $out_idx, 1) =~ /\s/) { $out_idx++; }

	return substr($cmd_output, $out_idx);
}

sub _send_file_to_linux {
	my $self = shift;
	my $lab_file_path = shift;
	my $galileo_file_path = shift;
	my $server = shift;

	my $backend = $self->{'backend_name'};
	unless(defined($backend)) {
		_error_msg("Backend is not specified\n");
		return undef;
	}

	unless(-e $lab_file_path) {
		_error_msg("File $lab_file_path not found.\n\n");
		return undef;
	}

	unless(defined($self->{'csconsole'})) {
		_error_msg("Console connection to $backend not opened.\n");
		return undef;
	}

	_print_msg("Sending $lab_file_path to $backend\n");

	my $dl_results = _download_xinu_image($backend, $lab_file_path);
	if(defined($dl_results)) {
		_error_msg("Unable to download file:$lab_file_path to backend: $backend\n");
		_error_msg($dl_results);
		return undef;
	}

	my $tftp_cmd = "tftp -g -r /tftpboot/$backend.xbin -l $galileo_file_path $server";
	
	my $tftp_cmd_output = _send_linux_cmd($self, $tftp_cmd);
	$tftp_cmd_output =~ s/[^[:print:]^\n]//g;
	if(length($tftp_cmd_output) > 0) {
		_error_msg("File transfer to $backend failed\n");
		_error_msg($tftp_cmd_output);
		_close_backend_connection($self);
		return undef;
	}

	return 1;
}

sub run_image {
	my $self = shift;
	my $image_name = shift;
	my $results_file = shift;

	add_xinu_image_to_run($self, $image_name);
	run_images($self, $results_file);
}

sub run_images {
	my $self = shift;
	my $results_file = shift;
	my $backend_type = $self->{'backend_type'};

	my $results_handle;

	# Set the results path
	#   Append to target directory unless user passed in full path (begin with '/')
	if(defined($results_file)) {
		my $results_path = $results_file;
		unless($results_file =~ /^\//) {
			$results_path = $self->{'target_directory'} . "/$results_path"
		}
		#if(-e $results_path) {
		#	_munlink($results_path);
		#}	
		open $results_handle, ">>", $results_path or die "Unable to open $results_path: $!\n";
		_print_msg("run_images :: Results printing to $results_path\n");
	} else {
		$results_handle = *STDOUT;
		_print_msg("run_images :: Results printing to STDOUT\n");
	}

	unless(defined(_open_backend_connection($self))) {
		print STDERR "Unable to open a connection to a backend\n";
		if(defined($results_file)) {
			close $results_handle;
		}
		return;
	}
	my $backend = $self->{'backend'};

	for(sort keys %{$self->{'images'}}) {

		# Set the image path
		#   Append to target directory unless user passed in full path (begin with '/')
		my $image_path = $_;
		unless($image_path =~ /^\//) {
			$image_path = $self->{'target_directory'} . "/$image_path"
		}

		print "Running $image_path on backend $backend\n";

		print $results_handle "\n\n-------------------------------------\n";
		print $results_handle "OUTPUT FOR $image_path\n";

		unless(-e $image_path) {
			print $results_handle "NO image $image_path found.\n\n";
			next;
		}

		my $dl_results = _download_xinu_image($backend, $image_path);
		if(defined($dl_results)) {
			print $results_handle "Unable to download image to backend: $backend:\n";
			print $results_handle $dl_results;
			next;
		}

		my $run_results;
		my $retries = 0;
		while($retries < $self->{'retries'}) {	

			_print_msg("\nRun attempt ended in error, retrying $retries, max retries " . $self->{'retries'} . "...\n") unless($retries == 0);

			my $pc_results = _power_cycle_backend($backend);
			if(defined($pc_results)) {
				print $results_handle "Unable to power cycle backend: $backend:\n";
				print $results_handle $pc_results;
				last;
			}

			$run_results = $platform_info{$self->{'backend_type'}}($self);
			if(defined($run_results)) {
				last;
			}

			$retries++;
		}

		if($retries >= $self->{'retries'}) {
			print $results_handle "Failed to run $image_path on backend: $backend due to too many failures\n";
		} else {
			print $results_handle "$run_results\n";
		}
	}

	_close_backend_connection($self);

	if(defined($results_file)) {
		close $results_handle;
	}
}

sub retrieve_backend_status {
	my $backend_type = shift;

	my $status_cmd = "/p/xinu/bin/cs-status -c $backend_type";
	_print_msg("retrieve_backend_status :: $status_cmd\n");

	my $return_status;

	for(split(/\n/, `$status_cmd`)) {
		if(/\s+(\w+)\s+$backend_type\s+user= (\w+)\s+time= ([0-9:]+)/) {
			my $backend_status;
			$backend_status->{'user'} = $2;
			$backend_status->{'time'} = $3;

			$return_status->{$1} = $backend_status;
			next;
		}
		if(/\s+(\w+)\s+$backend_type\s+user=\s+time=/) {
			$return_status->{$1} = undef;
			next;
		}
	}

	return $return_status;
}

sub _handle_quark_run {
	my $self = shift;

	# Skip passed the Galileo boot up output
	_print_msg("_handle_quark_run :: Waiting for " . $self->{'backend'} . " to start\n");
	
	my $patidx = $self->{'csconsole'}->expect(60, ("Booting 'Xboot'"));
	unless(defined($patidx)) {
		my $error_data = $self->{'csconsole'}->before();
		$error_data =~ s/[^[:print:]^\n]//g;
		print STDERR $self->{'backend'} . " Run failed due to unexpected output\n";
		print STDERR $error_data;
		return undef;
	}

	my $results = "Xinu for galileo";

	_print_msg("_handle_quark_run :: " . $self->{'backend'} . " started, retrieving output\n");
	my $patidx = $self->{'csconsole'}->expect(60, ("DHCP failed to get response", "Xinu for galileo"));
	unless(defined($patidx)) {
		my $error_data = $self->{'csconsole'}->before();
		$error_data =~ s/[^[:print:]^\n]//g;
		print STDERR $self->{'backend'} . " Run failed due to unexpected output\n";
		print STDERR $error_data;
		return undef;
	}
	if($patidx == 1) {
		print STDERR "Run failed due to DHCP timeout\n";
		return undef;
	}

	$results .= _read_and_format_data($self);
	return $results;
}

sub _expect_close_on_error {
	my $self = shift;
	my $timeout = shift;
	my $matches = shift;

	unless(defined($matches)) {
		$matches = $timeout;
		$timeout = $self->{'default_timeout'};
	}

	my $patidx = $self->{'csconsole'}->expect($timeout, @{$matches});
	unless(defined($patidx)) {
		my $error_data = $self->{'csconsole'}->before();
		$error_data =~ s/[^[:print:]^\n]//g;
		print STDERR $self->{'backend'} . " unexpected output\n";
		print STDERR $error_data;
		_close_backend_connection($self);
		return undef;
	}
	return $patidx;
}

sub _retrieve_open_backend {
	my $backend_type = shift;

	my $backend_status = retrieve_backend_status($backend_type);
	for(sort keys %{$backend_status}) {
		return $_ unless(defined($backend_status->{$_}->{'user'}));
	}
	return undef;
}

sub _open_backend_connection {
	my $self = shift;	

	my $backend_type = $self->{'backend_type'};

	my $backend = $self->{'backend_name'};
	unless(defined($self->{'backend_name'})) {
		$backend = _retrieve_open_backend($backend_type);
	}

	unless(defined($backend)) {
		return undef;
	}

	my $open_cmd = "/p/xinu/bin/cs-console -f $backend";

	_print_msg("$open_cmd\n");

	my $csconsole = Expect->spawn($open_cmd);
	$csconsole->log_user(0);

	my $result_idx = $csconsole->expect(10, ("connection '$backend', class '$backend_type', host", "error:"));
	unless(defined($result_idx)) {
		return undef;
	}

	if($result_idx == 2) {
		$csconsole->soft_close();
		return undef;
	}

	$self->{'csconsole'} = $csconsole;
	$self->{'backend'} = $backend;
	return 1;
}

sub _close_backend_connection {
	my $self = shift;

	_print_msg("Closing connection to " . $self->{'backend'} . "\n");

	return unless(defined($self->{'csconsole'}));

	$self->{'csconsole'}->send("\000");
	if($self->{'csconsole'}->expect(10, ("(command-mode) ")) == 1) {
		$self->{'csconsole'}->send("q");
	}
	$self->{'csconsole'}->soft_close();
	undef $self->{'csconsole'};
	undef $self->{'backend'};
}

sub _power_cycle_backend {
	my $backend = shift;

	my $power_cycle_cmd = "/p/xinu/bin/cs-console -t -f -c POWERCYCLE $backend-pc";
	_print_msg("$power_cycle_cmd\n");

	my $results = `$power_cycle_cmd`;
	if($results =~ /error/) {
		return $results;
	}
	return undef;
}

sub _download_xinu_image {
	my $backend = shift;
	my $image_path = shift;

	my $download_cmd = "/p/xinu/bin/cs-console -t -f -c DOWNLOAD $backend-dl < $image_path";
	_print_msg("$download_cmd\n");

	my $results = `$download_cmd`;

	unless($results =~ /cp-download complete/) {
		return $results;
	}
	return undef;
}

sub _read_and_format_data {
	my $self = shift;

	my $prev_length = 0;
	my $results;

	my $timeout = $self->{'default_timeout'};
	if(defined($self->{'timeout'})) {
		$timeout = $self->{'timeout'};
	}

	my $max_wait = $self->{'default_max_wait'};
	if(defined($self->{'max_wait'})) {
		$max_wait = $self->{'max_wait'};
	}

	_print_msg("Using timeout: $timeout sec\n");
	_print_msg("Using max wait: $max_wait sec\n");

	my $start_time = time;

	my $keep_reading = 1;
	while($keep_reading == 1) {
		$keep_reading = 0;

		$self->{'csconsole'}->expect($timeout);
		$results = $self->{'csconsole'}->before();
		if((length $results) > $prev_length) {
			$prev_length = length $results;
			$keep_reading = 1;
		}

		if((time - $start_time) > $max_wait) {
			_print_msg("Max wait time exceeded\n");
			$keep_reading = 0;
		}
	}

	$results =~ s/[^[:print:]^\n]//g;

	return $results;
}

sub _munlink {
	my $file = shift;
	unlink $file or die "Unable to remove file $file: $!\n";
}

sub _print_msg {
	if(VERBOSE) {
		my (
			$package, $filename, $line, $subroutine, $hasargs, 
			$wantarray, $evaltext, $is_require, $hints, $bitmask, $hinthash
		) = caller(1);

		print "$subroutine :: @_";
	}
}

sub _error_msg {
	my (
		$package, $filename, $line, $subroutine, $hasargs, 
		$wantarray, $evaltext, $is_require, $hints, $bitmask, $hinthash
	) = caller(1);

	print STDERR "$subroutine :: @_";
}

return 1;
