#!/usr/bin/perl

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

GetOptions (
            "verbose"  => \$verbose, # flag
			"help"     => sub {
			print <<HELP;
Uasge: img2lcd.pl [-v]

Options:
--verbose         Be verbose.
HELP
			exit(-1);}
			);


###
### Code starts here.
###

my $mode=shift;

my $in=shift || "i42.gif";


my $out=shift || $in;
$out=~s/\..*/.lcd/;

load GD;
my $image = GD::Image->new($in);

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
open(F,">",$out.".h")||die "open: $!";

print F "unsigned int img${mode}_len = ",scalar(@img),";\n";
print F "const unsigned char img${mode}_raw[] = {\n";

my $le;
if($mode==12){
	$le=130*1.5;
}elsif($mode==16){
	$le=130*2;
}else{
	die;
};
my $ctr=0;
for (@img){
#		printf F "%c",$img[$w-$x-1][$hb-$y];
	printf F "0x%02x, ",$_;
	printf Q "%c",$_;
	if (++$ctr%$le ==0) {
		print F "\n";
	};
};
print F "};\n";

close(F);
close(Q);
