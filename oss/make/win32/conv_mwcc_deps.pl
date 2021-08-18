#!/bin/perl

# Convert Metrowerks ".dep" dependency files into a ".d" file that can
# be understood by GNU make.
# This involves changing the paths to use unix-style paths, with no
# embedded spaces (because that seems to confuse GNU make)

use strict;
use FileHandle;
use IPC::Open2;

my $first = 1;

# pre-program regexp -- this will interpret the escape char ("\"), so for
# every backslash we wish to have in the actual regexp, we must program 2
# in the string. So the 6 backslashes in there actually resolve to 3 in the
# regexp (which is interpreted as '\\' followed by '\w', ie: match a backslash
# followed by a word char).
my $fnameRegexp = "([a-zA-Z]:)?((\\\\[\\w\\.])?[-_\\.\\w]*(\\\\ )?)+";

my $pid1 = open2( *toWinOut, *toWinIn, "cygpath -w -s -f -" );
my $pid2 = open2( *toUnixOut, *toUnixIn, "cygpath -u -f -" );

while ( <> ) {
  chomp;
  my $fname;
  my $contline;
  if ( $first == 1 ) {
    $first = 0;
    my $target;
    /\s*(\w*(.\w*|))\s*:\s*($fnameRegexp)\s*(\\?)/o;
    $target = $1;
    $fname = $3;
    $contline = $8;
    print "$target: ";
  } else {
    /\s*($fnameRegexp)\s*(\\)?/o;
    $fname = $1;
    $contline = $6;

    print "\t";
  }

  # Replace escaped-spaces with plain-old spaces in filename
  $fname =~ s/\\ / /g;

##  my $cmd = "cygpath -w -s \"$fname\"";
##  print "**Cmd = $cmd **\n";
##  my $fnameShort = `$cmd`;
  print toWinIn "$fname\n";
  my $fnameShort = <toWinOut>;
  chomp $fnameShort;

##  print "$fnameShort $contline\n";

##  $cmd = "cygpath -u \"$fnameShort\"";
##  my $fnameUnix = `$cmd`;
##  chomp $fnameUnix;

  print toUnixIn "$fnameShort\n";
  my $fnameUnix = <toUnixOut>;
  chomp $fnameUnix;

  print "$fnameUnix $contline\n";
}
