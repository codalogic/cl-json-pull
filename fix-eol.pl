#!/usr/bin/perl

@files = qw( include/dsl-pa/dsl-pa.h
			include/dsl-pa/dsl-pa-dsl-pa.h
			include/dsl-pa/dsl-pa-reader.h
			include/dsl-pa/dsl-pa-alphabet.h
			include/dsl-pa/dsl-pa-lite.h
			src/dsl-pa-dsl-pa.cpp
			src/dsl-pa-reader.cpp
			src/dsl-pa-alphabet.cpp
			test/clunit.h
			test/main-test.cpp
			test/dsl-pa-test.cpp
			test/reader-test.cpp
			test/alphabet-test.cpp
			examples/dsl-pa-examples.cpp
			README.md );
				
@files = @ARGV if $#ARGV >= 0;	# $#ARGV is the index of the last member, which may be -1

foreach $file (@files) {
	print "$file\n";
	system( "/home/pete/bin/itabs.exe", $file );
	if( open( FOUT, '>temp-eol.txt' ) && open( FIN, $file ) ) {
		while( <FIN> ) {
			s/\s*$//;
			print FOUT "$_\n";
		}
		close FIN;
		close FOUT;
		rename( 'temp-eol.txt', $file );
	}
}
unlink( 'temp-eol.txt' );
