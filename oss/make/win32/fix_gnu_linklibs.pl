#!/bin/perl

#
# Fix link-libs for GNU compiler/linker
#
# Problem: GNU linker will only consider libraries names "libXXX.a" or
# "XXX.dll" when searching for a match for the "-lXXX" link-line argument.
# But on NT, in general, import libs are named "XXX.lib", and the
# corresponding "XXX.dll" dynamic libs contain no exported symbols.
# Thus the linker fails.
#
# This script accepts a list of paths, introduced by the -path arg
# and a list of library basenames, introduced by the -lib arg. Note that
# these must be one of 2 types of names:
# 1) basenames only, ie: no ".lib" or ".a" suffix.
# 2) full names, ie: including the ".lib" suffix
#
# The script searches all paths (in the order given), looking for, in
# order of preference:
# 1) libXXX.a for basename XXX
#    If found, the specified lib basename is output to standard out,
#    prefixed with "-l" (ie: ready for use on the link command line)
# 2a) libXXX.lib or XXX.lib for basename XXX, or
# 2b) XXX, for specified full name XXX 
#    If found, output the full name of the object, including the directory,
#    to standard out. When used on the link command line, this will look
#    like an object, rather than a library, but it will work.
# 3) if nothing else is found, simply output the basename prefixed by
#    "-l" -- this assumes that the library exists somewhere in a system
#    or otherwise un-specified path.
#
#
# Usage:
#
# fix_gnu_linklibs -path PATH1 PATH2 ... -lib name1 name2 ...
#
use strict;

#
# Parse command-line args. Non-standard arg usage, can't use getopt.
#
my @paths;
my @libs;

my $readingPathOrLib = 0; ## 1=PATH, 2=LIB, 0=NEITHER

for ( my $i = 0; $i <= $#ARGV; $i++) {

  if ($ARGV[$i] =~ /^-/) {
    if ($ARGV[$i] =~ /^-path/) {
      $readingPathOrLib = 1;
    } elsif ($ARGV[$i] =~ /^-lib/) {
      $readingPathOrLib = 2;
    } else {
      die( "Unrecognised option $ARGV[$i].\n" );
    }

  } elsif ($readingPathOrLib == 1) {

    $paths[++$#paths] = $ARGV[$i];

  } elsif ($readingPathOrLib == 2) {

    $libs[++$#libs] = $ARGV[$i];

  } else {
    die( "Must use -path and -lib arguments.\n" );
  }
} ## for

# If no libs were specified, simply exit right now
if (@libs == 0) {
  exit 0;
}

# If no paths were specified, continue -- the loop below will simply
# do nothing, the libs will not be found, and will end up generating
# -lXXX, which should be the "default action".

foreach my $lb (@libs) {

  my $dotAName = "lib$lb.a";
  my $dotLibName1 = "$lb.lib";
  my $dotLibName2 = "lib$lb.lib";

  my $found = 0;
  for ( my $i=0; ($i <= $#paths) && ($found == 0); $i++ ) {

    my $pth = $paths[$i];

    if ( -e "$pth/$dotAName" && ! -d "$pth/$dotAName" ) {
      # Found a regular library, output -lXXX
      print " -l$lb";

      # Stop searching
      $found = 1;
    } elsif ( (-e "$pth/$dotLibName1" && ! -d "$pth/$dotLibName1") ) {
      # Found a Windows-style import lib.
      # Construct full path name to this object, and output that.
      print " $pth/$dotLibName1";

      # Stop searching
      $found = 1;

    } elsif ( (-e "$pth/$dotLibName2" && ! -d "$pth/$dotLibName2") ) {
      # Found a Windows-style import lib.
      # Construct full path name to this object, and output that.
      print " $pth/$dotLibName2";

      # Stop searching
      $found = 1;

    } elsif ( (-e "$pth/$lb" && ! -d "$pth/$lb") ) {
      # Found object with specified full name -- assume this is a valid
      # library
      # Construct full path name to this object, and output that.
      print " $pth/$lb";

      # Stop searching
      $found = 1;
    }
  } # for i=0..$#paths

  # If nothing was found, output "-l$lb", ie: assume a standard ".a" style
  # library exists somewhere the linker can find it.
  if ( $found == 0 ) {
    print " -l$lb";
  }

} # foreach libs

print "\n";




