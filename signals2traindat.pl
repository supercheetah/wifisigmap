#!/usr/bin/perl

use strict;
my $file_in = shift || "signals.dat";
my $file_out_prefix = shift || "signals-training";

my @input_data = `cat $file_in`;

my %ap_data = ();

my $cur_mac = undef;

foreach my $line (@input_data)
{
	$line =~ s/[\r\n]//g;
	$line =~ s/(^\s+|\s+$)//g;
	next if !$line;

	if($line =~ /:/)
	{
		# Mac header
		my ($mac,$essid) = split /,/, $line;
		$cur_mac = $mac;
		# ignore essid
		$ap_data{$mac} = [];
	}
	elsif($line =~ /^dBm/)
	{
		next;
	}
	else
	{
		#dBm,Signal%,Dist(Meters),CalcDist(Meters)
		my ($dbm,$sig,$dist,$calc) = split /,/, $line;

		$dbm  = ($dbm + 150) / 150;
		$calc = $calc / 1000;
		$dist = $dist / 1000;
		
		#push @{$ap_data{$cur_mac}}, [ [ $dbm, $sig, $calc ], [ $dist ] ];
		#push @{$ap_data{$cur_mac}}, [ [ $dbm ], [ $dist ] ];
		push @{$ap_data{$cur_mac}}, [ [ $dbm, $sig ], [ $dist ] ];
	}
}

use Data::Dumper;
#die Dumper \%ap_data;
my $TRAIN_CMD = './train_net';

foreach my $ap (keys %ap_data)
{
	my @data = @{$ap_data{$ap}};

	# skip aps with no data
	next if !@data;
	
	# Open output file
	my $mac_sans = $ap;
	$mac_sans =~ s/://g;
	my $out_file = "$file_out_prefix-$mac_sans.train";
	open(FILE, ">$out_file") || die "Cannot open $out_file for writing: $!";

	# Print header
	my ($in,$out) = @{$data[0] || [[],[]]};
	print FILE scalar(@data), ' ',
		   scalar(@$in),  ' ',
		   scalar(@$out), "\n";

	# Print pairs of training data
	foreach my $pairs (@data)
	{
		my ($in,$out) = @$pairs;
		print FILE join(' ', @$in),  "\n";
		print FILE join(' ', @$out), "\n";
	}
	close(FILE);
	
	print "Wrote ".scalar(@data)." lines to $out_file\n";

	if(-f $TRAIN_CMD && -x $TRAIN_CMD)
	{
		my $net_file = "signals-$mac_sans.net";
		print "Training $net_file\n";
		system("$TRAIN_CMD $out_file $net_file");

		#exit(-1); # just testing...
	}
}

