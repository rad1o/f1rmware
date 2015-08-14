#!/usr/bin/env perl

# img2lcd.pl - by <sec@42.org> 05/2011, BSD Licence
#
# This script converts an image to .lcd format for the rad1o

use strict;
use warnings;
use Getopt::Long;
use Module::Load;

$|=1;

###
### Runtime Options
###

my ($verbose);
my ($mode);
my ($header);

$mode=12;

GetOptions (
            "verbose"   => \$verbose, # flag
            "bits=i"    => \$mode,    # numeric
            "code"      => \$header,  # flag
			"help"     => sub {
			print <<HELP;
Uasge: img2lcd.pl [-v] [-b 8|12|16] <inputfile> [<outputfile>]

Options:
--verbose       Be verbose.
--code          Generate C header
--bits=n        How many bits per pixel
HELP
			exit(-1);}
			);

###
### Code starts here.
###

my $in=shift || "i42.gif";

my $out=shift;

if( !defined $out){
    $out=$in;
    $out=~s/\.[^.]*$/.lcd/;
};

load GD;
my $image = GD::Image->new($in) || die "Could not open $in: $!\n";

my $w=$image->width;
my $h=$image->height;

$w=130;
$h=130;

if($verbose){
	print STDERR "$in: ${w}x$h\n\n";
};

my @img;
my $odd=0;
my $keep;
my $ct=0;
for my $y (0..$h-1){
	$ct++;
	for my $x (0..$w-1){
		my $px= $image->getPixel($x,$y);
		my ($r,$g,$b) = $image->rgb($px);

		if ($mode==12){
			$r>>=4;
			$g>>=4;
			$b>>=4;

			if ($odd){
				push @img, ($keep<<4|$r);
				push @img, ($g<<4|$b);
				$odd=0;
			}else{
				push @img, ($r<<4|$g);
				$keep=$b;
				$odd=1;
			};
		}elsif($mode==8){
			push @img,($r&0b11100000)|($g&0b11100000)>>3|($b&0b11000000)>>6;
		}elsif($mode==16){
			push @img,(($r&0b11111000)|($g>>5));
			push @img,(($g&0b00011100)<<3|($b>>3));
		}else{
			die "This mode is not implemented yet.";
		};
	};
};

print "Size:",scalar(@img),"\n" if ($verbose);

open(Q,">",$out)||die "open: $!";
if($header){
open(F,">",$out.".h")||die "open: $!";

print F "unsigned int img${mode}_len = ",scalar(@img),";\n";
print F "const unsigned char img${mode}_raw[] = {\n";
};

my $le;
if($mode==12){
	$le=130*1.5;
    unshift @img,2;
}elsif($mode==16){
	$le=130*2;
    unshift @img,3;
}elsif($mode==8){
	$le=130;
    unshift @img,1;
}else{
	die;
};
my $ctr=0;
for (@img){
	printf F "0x%02x, ",$_ if $header;
	printf Q "%c",$_;
	if (++$ctr%$le ==0) {
		print F "\n" if $header;
	};
};
print F "};\n" if $header;

close(F) if $header;
close(Q);
