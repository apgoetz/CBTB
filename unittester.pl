#!/usr/bin/perl

use strict;
use warnings;
sub exec_test;
sub process_args {
  my $testdir = "";
  my $args = shift;
  my $arg = "";
  for (my $i = 0; $i < scalar @$args; $i++) {
    $arg = $args->[$i];
    if($arg =~ /^-h/i) {
      print "Usage: $0 test_directory\n";
      exit 0;
    } else {
      $testdir = $arg;
    }
  }
  if ("$testdir" eq "") {
    print "Usage: $0 test_directory\n";
    exit 0;
  }
  return $testdir;
}

my $testdir = process_args(\@ARGV);

opendir (my $dir, $testdir) or die ("Could not open test directory $testdir: $!");
my @testlist = grep(!/(^\.|~)/,readdir $dir);
closedir $dir;

my $total_tests = scalar @testlist;

my @retvals = map(exec_test($_,$testdir), @testlist);
print "passed ";
print scalar grep($_, @retvals);
print " of $total_tests tests.\n";

sub exec_test {
  my $testname = shift;
  my $testdir = shift;
  my $testfile = "$testdir/$testname";
  print "Testing: $testname ";
  
  open my $fh, '<', "$testfile" or die;
  my $line = '';
  my @trace;
  do {
    $line = <$fh>;
    next if $line =~ /^\w*#/;
    push @trace, $line;
  } while($line !~ /^=/);

  my %asserts;
  while(<$fh>) {
    next if /^\w*#/;
    if(/^(\w+)\W*:\W*(\w+)/) {
      $asserts{$1} = $2;
    }
  }
  my @rawresults = `./tester < $testfile 2>/dev/null`;
  my %results;
  foreach (@rawresults) {
    if (/^(\w+)\W*:\W*(\w+)/) {
      $results{$1} = $2;
    }
  }
  my $passed = 1;
  my $printedstatus = 0;
  foreach (keys %asserts) {
    if (!exists $results{$_}) {
      print "[WARN]\n" if (!$printedstatus);
      print "WARNING: No assert key '$_' in test $testname\n";
      $printedstatus = 1;
      next;
    }
    if ($results{$_} != $asserts{$_}) {
      print "[FAIL]\n" if (!$printedstatus);
      print "FAIL: Expected ".$asserts{$_}." and received ".$results{$_};
      print " for key '$_' in test $testname\n";
      $passed = 0;
      $printedstatus = 1;
    }
  }

  close $fh;
  print "[OK]\n" if (!$printedstatus);
  return $passed;
}
