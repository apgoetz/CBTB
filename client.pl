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


my @filenames = ("traces/DIST-INT-1", "traces/DIST-MM-1");
my $sum = 0.0;
foreach (@filenames) {
  my $result;
  if (`./launcher.pl tester $commandline $_ | grep missed_targets` =~ /(\d+\.\d+)$/) {
    $result = "$1"
  } else {
    $result = "100.0";
  }
  $sum += $result*1.0;
}
$sum /= scalar @filenames;
print $socket "$commandline,$sum\n";
close $socket;
