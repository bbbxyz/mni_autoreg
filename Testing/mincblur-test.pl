#! /usr/bin/env perl
#
# Tests for mincblur
#


# Test individual options

use strict;
my $errors=0;

my $mincblur_bin= "mincblur";
chomp($mincblur_bin);
if ($ENV{'MINCBLUR_BIN'})
{
    $mincblur_bin = $ENV{'MINCBLUR_BIN'};
}


print "Case 1 - Test help message.\n";

my $r1 = `$mincblur_bin -help 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c1result = 'Command-specific options:
 -fwhm:         Full-width-half-maximum of gaussian kernel
                Default value: 0
 -standarddev:  Standard deviation of gaussian kernel
                Default value: 0
 -3dfwhm:       Full-width-half-maximum of gaussian kernel
                Default value: -1.79769e+308 -1.79769e+308 -1.79769e+308
 -dimensions:   Number of dimensions to blur (either 1,2 or 3).
                Default value: 3
Program flags.
 -no_clobber:   Do not overwrite output file (default).
 -clobber:      Overwrite output file.
 -gaussian:     Use a gaussian smoothing kernel (default).
 -rect:         Use a rect (box) smoothing kernel.
 -gradient:     Create the gradient magnitude volume as well.
 -partial:      Create the partial derivative and gradient magnitude volumes as well.
 -no_apodize:   Do not apodize the data before blurring.
Options for logging progress. Default = -verbose.
 -verbose:      Write messages indicating progress
 -quiet:        Do not write log messages
 -debug:        Print out debug info.
 -version:      Print out version info and exit.
Generic options for all commands:
 -help:         Print summary of command-line options and abort
 -version:      Print version number of program and exit

Usage: mincblur [<options>] <inputfile> <output_basename>
       mincblur [-help]

';
if ($r1 ne $c1result)
{
    print "Case 1 failed, incorrect output format.\n";
    print "$r1";
    print "$c1result";
    $errors++;
}

print "Case 2 - Test -fwhm option.\n";

my $r2 = `$mincblur_bin -clobber -fwhm 5 volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c2result = <<'HERE';
Making byte volume...
HERE
if ($r2 ne $c2result)
{
    print "Case 2 failed, incorrect output format.\n";
    print "$r2";
    print "$c2result";
    $errors++;
}

print "Case 3 - Test -standarddev option.\n";

my $r3 = `$mincblur_bin -clobber -standarddev 3 volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c3result = <<'HERE';
Making byte volume...
HERE
if ($r3 ne $c3result)
{
    print "Case 3 failed, incorrect output format.\n";
    print "$r3";
    print "$c3result";
    $errors++;
}

print "Case 4 - Test -3dfwhm option.\n";

my $r4 = `$mincblur_bin -clobber -3dfwhm 2 2 2 volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c4result = <<'HERE';
Making byte volume...
HERE
if ($r4 ne $c4result)
{
    print "Case 4 failed, incorrect output format.\n";
    print "$r4";
    print "$c4result";
    $errors++;
}

print "Case 5 - Test -dimensions option.\n";

my $r5 = `$mincblur_bin -clobber -fwhm 2 -dimensions 2 volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c5result = <<'HERE';
Making byte volume...
HERE
if ($r5 ne $c5result)
{
    print "Case 5 failed, incorrect output format.\n";
    print "$r5";
    print "$c5result";
    $errors++;
}

print "Case 6 - Test -rect  option.\n";

my $r6 = `$mincblur_bin -clobber -fwhm 2 -rect volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c6result = <<'HERE';
Making byte volume...
HERE
if ($r6 ne $c6result)
{
    print "Case 6 failed, incorrect output format.\n";
    print "$r6";
    print "$c6result";
    $errors++;
}

print "Case 7 - Test -gradient option.\n";

my $r7 = `$mincblur_bin -clobber -fwhm 2 -gradient volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c7result ='Transforming slices:...........................................................................................Done
Making byte volume...
Making byte volume dx...Making byte volume dy...Making byte volume dz...';
if ($r7 ne $c7result)
{
    print "Case 7 failed, incorrect output format.\n";
    print "$r7";
    print "$c7result";
    $errors++;
}

print "Case 8 - Test -partial option.\n";

my $r8 = `$mincblur_bin -clobber -fwhm 2 -partial volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c8result ='Transforming slices:...........................................................................................Done
Making byte volume...
Making byte volume dx...Making byte volume dy...Making byte volume dz...';
if ($r8 ne $c8result)
{
    print "Case 8 failed, incorrect output format.\n";
    print "$r8";
    print "$c8result";
    $errors++;
}

print "Case 9 - Test -gaussian option.\n";

my $r9 = `$mincblur_bin -clobber -fwhm 2 -gaussian volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c9result = <<'HERE';
Making byte volume...
HERE
if ($r9 ne $c9result)
{
    print "Case 9 failed, incorrect output format.\n";
    print "$r9";
    print "$c9result";
    $errors++;
}

print "Case 10 - Test -no_apodize option.\n";

my $r10 = `$mincblur_bin -clobber -fwhm 2 -no_apodize volume_a_small.mnc volume_a 2>&1`;
if ($? != 0)
{
    print "Default operation returned something other than zero: $?\n";
    $errors++;
}
my $c10result = <<'HERE';
Making byte volume...
HERE
if ($r10 ne $c10result)
{
    print "Case 10 failed, incorrect output format.\n";
    print "$r10";
    print "$c10result";
    $errors++;
}

# Test cases that should fail

print "Case 11 - Test missing 3dfwhm dimension.\n";

my $r11 = `$mincblur_bin -clobber -3dfwhm 2 2 -volume_a_small.mnc volume_a 2>&1`;
if ($? == 0)
{
    print "Case that should fail returned 0\ n";
    $errors++;
}

print "Case 12 - Test contradicting kernel sizes.\n";

my $r12 = `$mincblur_bin -clobber -3dfwhm 2 2 2 -fwhm 7 -volume_a_small.mnc volume_a 2>&1`;
if ($? == 0)
{
    print "Case that should fail returned 0\ n";
    $errors++;
}

print "Case 13 - Test contradicting kernel sizes.\n";

my $r13 = `$mincblur_bin -clobber -3dfwhm 2 2 2 -standarddev 7 -volume_a_small.mnc volume_a 2>&1`;
if ($? == 0)
{
    print "Case that should fail returned 0\ n";
    $errors++;
}
print "Case 14 - Test contradicting kernel sizes.\n";

my $r14 = `$mincblur_bin -clobber -standarddev 2 -fwhm 7 -volume_a_small.mnc volume_a 2>&1`;
if ($? == 0)
{
    print "Case that should fail returned 0\ n";
    $errors++;
}


print "OK.\n" if $errors == 0;
print
exit $errors > 0;


