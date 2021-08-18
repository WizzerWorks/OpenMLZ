#!/bin/perl

use strict;

use Getopt::Long;

my $defFile = undef;
my $symFile = undef;
my $verbose = undef;

GetOptions ( "d=s" => \$defFile,
	     "s=s" => \$symFile,
	     "verbose" => \$verbose
	   )
|| die( "Error in command-line.\n" );

if( ! defined( $defFile ) ) {
  die( "Must specify input .def file with -d switch\n" );
}

if( ! defined( $symFile ) ) {
  die( "Must specify input symbol file with -s switch\n" );
}

# First step: read in symbols, extract all those that appear "interesting",
# and keep them in a hash, along with the corresponding stack size.
#
# An "interesting" symbol is one whose name is mangled according to
# "__stdcall" rules, ie: it has a @xx suffix, where xx is the function's
# stack size. It also has a leading "_" (which we strip before storing in
# the hash).
my %symbols;

open( SYM, $symFile ) || die( "Can not open sym file $symFile.\n" );

while (<SYM>) {
  chomp;

  if ( /^[0-9a-fA-F]+\s+T\s+_(\w+)@(\d+)/ ) {
    my $symbol = $1;
    my $stackSize = $2;

    if( defined( $verbose ) ) {
      print "*** Symbol = $symbol, stack size = $stackSize\n";
    }

    $symbols{$symbol} = $stackSize;
  }
}

# Done with symbols file, close it.
close( SYM );

open( DEF, $defFile ) || die( "Can not open def file $defFile.\n" );

while (<DEF>) {
  chomp;

  # Initially, assume we will simply be echoing the incoming line as-is
  my $outLine = $_;

  # Ignore comments and known keywords (but not the LIBRARY keyword --
  # see below)
  if ( ! /^(;|DESCRIPTION|EXPORTS)/ ) {

    # Is it the "LIBRARY" keyword? If so, we must make sure the library
    # name includes the ".dll" suffix
    if ( /^LIBRARY\s+\"(\w+)\"/ ) {
      my $libName = $1;
      $outLine = "LIBRARY  \"$1.dll\"";

    } elsif ( /^(\w+)/ ) {
      # Attempt to extract a symbol
      my $symbol = $1;

      if( defined( $symbols{$symbol} ) ) {

	if( defined( $verbose ) ) {
	  print "*** Found symbol = $symbol, stack size = $symbols{$symbol}\n";
	}
	$outLine = "$symbol" . "@" . "$symbols{$symbol}";

      }
    }
  }

  print "$outLine\n";
}

# Done
close( DEF );
