#!/usr/bin/perl

use strict;
use warnings;
use Socket;
use Carp;
use POSIX ":sys_wait_h";
use Errno;
# build the jobs data structures that are spawned off.

my @children_pids;
my %assignedjob;

sub build_cmd
{
    my $opts = shift;
    my $cmd = "";
    foreach my $k (keys %$opts){
	$cmd = $cmd.'--'.$k.'='.$opts->{$k}.' ';
    }
    return $cmd;
}


sub logmsg { print "$0 $$: @_ at ", scalar localtime(), "\n" }
sub print_child
{
    my $pid = shift;
    my $exitval = shift;
    print "j: " . $assignedjob{$pid} . " : ".($exitval>>8)."\n";
}

BEGIN { $ENV{PATH} = "/usr/bin:/bin" }

my $EOL = "\015\012";

sub spawn; # forward declaration



sub REAPER {
    local $!; # don't let waitpid() overwrite current error
    while ((my $pid = waitpid(-1, WNOHANG)) > 0 && WIFEXITED($?)) {
    }
    $SIG{CHLD} = \&REAPER; # loathe SysV
}

sub listener {
    my $jobs = shift;
    my $port = 12345;

    die "invalid port" unless  $port =~ /^ \d+ $/x;
    my $proto = getprotobyname("tcp");
    socket(Server, PF_INET, SOCK_STREAM, $proto) || die "socket: $!";
    setsockopt(Server, SOL_SOCKET, SO_REUSEADDR, pack("l", 1))
	|| die "setsockopt: $!";
    bind(Server, sockaddr_in($port, INADDR_ANY)) || die "bind: $!";
    listen(Server, SOMAXCONN) || die "listen: $!";
    logmsg "server started on port $port";
    my $paddr;
    

    $SIG{CHLD} = \&REAPER;
    
    print "num jobs: ".scalar @$jobs."\n";

    for (my $i = 0; $i < scalar @$jobs;) {
	$paddr = accept(my $Client, Server) || do {
# try again if accept() returned because got a signal
	    next if $!{EINTR};
	    die "accept: $!";
	};
	
	my $pid = spawn sub {
	    my $old_fh = select($Client);
	    $| = 1;
	    select($old_fh);
	    print $Client $jobs->[$i];
	    print $Client "\n";
	    my $response = <$Client>;
	    print "$response";
	};
	$assignedjob{$pid} = $i;
	close $Client;
	
	$i++;
    }
    my $loop = 1;
    $SIG{CHLD} = 'DEFAULT';  # turn off auto reaper
    $SIG{INT} = $SIG{TERM} = sub {$loop = 0; kill -15 => @children_pids};
    while ($loop && getppid() != 1) {
	my $child = waitpid(-1, 0);
	last if $child == -1;
    }
}
sub spawn {
    my $coderef = shift;
    unless (@_ == 0 && $coderef && ref($coderef) eq "CODE") {
	confess "usage: spawn CODEREF";
    }
    my $pid;
    unless (defined($pid = fork())) {
	logmsg "cannot fork: $!";
	return;
    }
    elsif ($pid) {
	push @children_pids, $pid;
	return $pid; # I'm the parent
    }
# else I'm the child -- go spawn
## open(STDERR, ">&STDOUT") || die "can't dup stdout to stderr";
    exit($coderef->());
}


# parse the contents of a status file, and turn into a data structure
sub parse_status
{
    my $filename = shift;
    open (my $fh, '<', $filename) or die "could not open file";
    my @options;
    while (<$fh>) {
      my @fields = split /,/;
      push @options, join (",", @fields[0..6]);
    }
    return \@options;
}

die "Usage: $0 baseconf" if (scalar @ARGV < 1);
#my $options = parse_status ($ARGV[0]);
#exit 0;
print "starting server\n";
listener(parse_status ($ARGV[0]));
