package XINUTestCase;

use strict;

use Data::Dumper;
use IO::Select;

sub new {
	my $class = shift;
	my $tgt_image = shift;
	
	my $self = {
		'image_name' => $tgt_image,
	};
	
	bless $self, $class;
	return $self;
}

sub add_file {
	my $self = shift;
	my $src = shift;
	my $tgt = shift;
	my $bkp = shift;
	
	$self->{'src_files'} = () unless defined($self->{'src_files'});
	$self->{'tgt_files'} = () unless defined($self->{'tgt_files'});
	$self->{'bkp_files'} = () unless defined($self->{'bkp_files'});
	
	my %src_h = map { $_->[0] => 1 } $self->{'src_files'};
	my %tgt_h = map { $_->[0] => 1 } $self->{'tgt_files'};
	my %bkp_h = map { $_->[0] => 1 } $self->{'bkp_files'};
	
	return if($src_h{$src});
	return if($tgt_h{$tgt});
	
	push @{$self->{'src_files'}}, $src;
	push @{$self->{'tgt_files'}}, $tgt;
	push @{$self->{'bkp_files'}}, $bkp;
}

sub image_name {
	my $self = shift;
	return $self->{'image_name'};
}

sub source_files {
	my $self = shift;
	return $self->{'src_files'};
}

sub target_files {
	my $self = shift;
	return $self->{'tgt_files'};
}

sub backup_files {
	my $self = shift;
	return $self->{'bkp_files'};
}

return 1;
