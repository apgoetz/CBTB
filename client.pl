#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;
use File::Temp qw /tempdir/;

die "Usage: $0 server\n" if (scalar @ARGV < 1);

my $packed_ip = gethostbyname($ARGV[0]);
my $ip_address = inet_ntoa($packed_ip);
my $socket = new IO::Socket::INET (
PeerHost => $ip_address,
PeerPort => '12345',
Proto => 'tcp',
) or die "ERROR in Socket Creation : $!\n";
my $commandline = <$socket>;
$commandline =~ s/\(/\\\(/g;
$commandline =~ s/\)/\\\)/g;
chomp $commandline;
my $hostname = `hostname -f`;
chomp $hostname;


my @filenames = (
    "traces/DIST-INT-1", 
    "traces/DIST-SERV-1",
    "traces/DIST-MM-1", 
    "traces/DIST-FP-1",

    "traces/DIST-INT-2", 
    "traces/DIST-SERV-2",
    "traces/DIST-MM-2", 
    "traces/DIST-FP-2",

    "traces/DIST-INT-3", 
    "traces/DIST-SERV-3",
    "traces/DIST-MM-3", 
    "traces/DIST-FP-3",

    "traces/DIST-INT-4", 
    "traces/DIST-SERV-4",
    "traces/DIST-MM-4", 
    "traces/DIST-FP-4",

    "traces/DIST-INT-5", 
    "traces/DIST-SERV-5",
    "traces/DIST-MM-5", 
    "traces/DIST-FP-5",
    );
my $sum = 0.0;
foreach (@filenames) {
  my $result;
  print "$_ $commandline\n";
  if (`./launcher.pl tester $commandline $_ oracle | grep missed_targets` =~ /(\d+\.\d+)$/) {
    $result = "$1"
  } else {
    $result = "100.0";
  }
  $sum += $result*1.0;
}
$sum /= scalar @filenames;
print $socket "$commandline,$sum\n";
close $socket;
