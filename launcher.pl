#!/usr/bin/perl

# helper script to count the number of possible cache configurations

#usage: ./launcher (tester|predictor) algo,mainsize,mainways,dispsize,dispways,dispentries,funccap tracefile

# cat cache_sizes | perl -ne '@d = split /,/; print unless (int($d[7]) \
# < 26000)' | sort -n -r -t, -k8 | head -10 | xargs -I '{}' \
# ./launcher.pl '{}' traces/DIST-INT-1.xz

exit (0) if (scalar @ARGV < 3);
$xzfile = $ARGV[2].".xz";
$cmd;
@d = split  /,/, $ARGV[1];
if ($ARGV[0] eq "tester") {
  $cmd = "xzcat $xzfile | ";
}


$cmd .=" ORACLE=$xzfile" if ($ARGV[3] =~ /oracle/i);
$cmd .=" BTB_WAY_ALGO=".$d[0];
$cmd .=" BTB_MAIN_SIZE=".$d[1];
$cmd .=" BTB_MAIN_WAYS=".$d[2];
$cmd .=" BTB_DISP_SIZE=".$d[3];
$cmd .=" BTB_DISP_WAYS=".$d[4];
$cmd .=" BTB_DISP_ENTRIES=".$d[5];
$cmd .=" BTB_FUNC_CAP=".$d[6];

 if ($ARGV[0] eq "tester") {
   $cmd .=" ./tester"
} else {
   $cmd .=" ./predictor ".$ARGV[2];
}
$cmd .=" 2>&1";
system $cmd;
