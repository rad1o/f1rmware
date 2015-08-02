#!/usr/bin/perl

# makefont.pl - by <sec@42.org> 05/2011, BSD Licence
#
# This script rasterizes a font at a given size, then compresses it using a
# simple RLE encoding and writes its definition into compilable .c/.h files

# Rasterisation is done via GD which in turn uses freetype
# Compression is done similar to the PK compressed fonts used by LaTex
# For a verbose description see:
# - <http://www.davidsalomon.name/DC4advertis/PKfonts.pdf> or
# - <http://www.tug.org/TUGboat/tb06-3/tb13pk.pdf>

# Font sources:
# - Google Webfonts: <https://www.google.com/webfonts/v2>
# - X11 system fonts (use pcf2pdf to convert to bdf)

use strict;
use warnings;
use Getopt::Long;
use Module::Load;

$|=1;

###
### Configure me
### 

my @charlist=(32..126,0x20ac); #,0x3044 # hiragana I
push @charlist,map {ord $_} qw(ä ö ü Ä Ö Ü ß);

###
### Runtime Options
###

my ($verbose,$raw,$chars,$bin);
my $size=18;

my $font="ttf/Ubuntu-Regular.ttf";

GetOptions ("size=i"   => \$size,    # numeric
            "font=s"   => \$font,    # string
            "verbose"  => \$verbose, # flag
			"raw"      => \$raw,     # flag
			"bin"      => \$bin,     # flag
			"chars=s"  => \$chars,   # list of chars
			"help"     => sub {
			print <<HELP;
Uasge: makefont.pl [-r] [-v] [-f fontfile] [-s size]

Options:
--verbose         Be verbose.
--raw             Create raw/uncompressed font.
--bin             Also create binary font file.
--font <filename> Source .ttf file to use. [Default: $font]
--chars <chars>   Characters to encode. [Deflault: see source :-)]
--size <size>     Pointsize the font should be rendered at. [Default: $size]
HELP
			exit(-1);}
			);

if($chars){
    @charlist=map {ord $_} split(//,$chars);
};

my ($type);
if($font=~/\.ttf/){
	$type="ttf";
}elsif($font=~/\.x?bdf/){
	$type="bdf";
}else{
	die "Can only do .ttf or .bdf fonts\n";
};

###
### Code starts here.
###

my $origsize;
my $c1size;
my $c2size;

our ($licence);
our ($title,$fonts);
our ($heightb,$heightpx);

@charlist=sort { $a <=> $b } @charlist;

$::{"init_$type"}();

die "No font name?" if !defined $title;

my $file=$fonts;
$file=~s/pt$//;
$file=~y/A-Z/a-z/;
#$file.="-raw" if($raw);

print "Writing $title to ${file}.c\n";

$heightb=int(($heightpx-1)/8)+1; # Round up
print "Chars are ",$heightpx,"px ($heightb bytes) high\n";

open (C,">",$file.".c")||die "Can't create $file.c: $!";

if(defined $licence){
	$licence=~s/\n/\n * /g;
	$licence="\n/* ".$licence."\n */";
}else{
	$licence="";
};

print C <<EOF
#include "$file.h"

/* Font data for $title */
$licence

/* This file created by makefont.pl by Sec <sec\@42.org> */

/* Bitmaps */
const uint8_t ${fonts}Bitmaps[] = {
EOF
;

my $offset=0;
my $maxsz=0;
my @offsets;
my (@bindata,@binoffsets);
for (0..$#charlist){
	my $char=chr $charlist[$_];
	print "### Start $char\n" if($verbose);

	my @char=$::{"render_$type"}($_);

	print C " /* Char ",ord $char," is ",scalar@char,"px wide \@ $offset */\n";

	$maxsz=scalar@char if scalar@char > $maxsz;

	$origsize+=$heightb * scalar @char;

	# Whoops. Characters are upside down.
	for (@char){
		$_=reverse $_;
	};

	### PK compression
	my @enc=do_pk(\@char);
	$c2size+=scalar(@enc);

	### Lame compression
	# "compress" blank spaces
	my $preblank=0;
	while(defined $char[1] && $char[0]=~/^0+$/){
		shift@char;
		$preblank++;
	};
	my $postblank=0;
	while($#char>0 && $char[$#char]=~/^0+$/){
		$#char--;
		$postblank++;
	};
	$c1size+=$heightb*scalar@char;

	my @raw;
	### Raw character data
	for (@char){
		my $h= pack("B*",$_).(chr(0).chr(0));
		for my $b (1..$heightb){
			push @raw,ord(substr($h,$b-1,1));
		};
	};

	# Maintenance overhead
	$origsize+=1;
	$c1size+=3;
	$c2size+=1;

	my $oneraw;
	# If encoding is bigger, fall back to original char
	if($#enc>$#raw+3){
		warn "Compression failure: Encoding char $char raw.\n" unless $raw;
		$oneraw=1;
	};

	# Generate C source
	if($raw||$oneraw){
        my @out;
		$c2size-=scalar(@enc);
        if(!$raw){
            @enc=(255,$preblank,$postblank);
            @out=@enc;
            push @bindata,@out;
            printf C "  0x%02x, %2d, %2d, /* rawmode, preblank, postblank */\n",
                   (shift@out), (shift@out), (shift@out);
        }else{
            @enc=();
        };
        push @enc,@raw;
		$c2size+=scalar(@enc);
		@out=@enc;
        push @bindata,@out;
		for (@char){
			print C "  ";
			printf C "0x%02x, ",shift@out for(1..$heightb);
			$_=~y/01/ */;
			print C " /* $_ */ \n";
		};
	}else{
		for (@char){
			$_=~y/01/ */;
			print C " /* $_ */ \n";
		};
		my $pretty=0;
        push @bindata,@enc;
		for(@enc){
			print C "  " if($pretty==0);
			printf C "0x%02x, ",$_;
			if(++$pretty==8){
				print C "\n" ;
				$pretty=0;
			};
		};
	};

	print C "\n";

    push @binoffsets,scalar(@enc);
	push @offsets,sprintf " {%2d}, /* %s */\n",scalar(@enc),$char;
	print C "\n";

	$offset+=scalar(@enc);

	if($verbose){
		print "-"x80,"\n";
	};
};


print C <<EOF;
};

/* Character descriptors */
const FONT_CHAR_INFO ${fonts}Lengths[] = {
EOF

print C @offsets;

my($first)=$charlist[0];
my($last)=$first-1;
for(@charlist){
	last unless $_ == $last+1;
	$last++;
};
print C <<EOF;
};

const uint16_t ${fonts}Extra[] = {
EOF

my @extras=(@charlist[($last-$first+1)..$#charlist],0xffff);
print C join(",",@extras);

printf C "
};

/* Font info */
const struct FONT_DEF Font_$fonts = {
	%3d,   /* width (1 == compressed) */
	%3d,   /* character height */
	%3d,   /* first char */
	%3d,   /* last char */
    %s, %s, %s
};
",($raw?0:1),$heightpx,$first,$last,"${fonts}Bitmaps","${fonts}Lengths","${fonts}Extra";

printf C "\n";
printf C "/* Font metadata: \n";
printf C " * Name:          %s\n", $title;
printf C " * Height:        %d px (%d bytes)\n", $heightpx,$heightb;
printf C " * Maximum width: %d px\n",$maxsz;
printf C " * Storage size:  %d bytes ",$c2size;
if($raw){
    printf C "(uncompressed)";
}else{
    printf C "(compressed by %2d%%)", (1-$c2size/$origsize)*100;
};
printf C "\n";
printf C " */\n";

close(C);

if($bin){
    open (B,">",$file.".f0n")||die "Can't create $file.f0n: $!";
    binmode(B); # Just to be safe.

    print B 
#    uint8_t u8Width;                /* Character width for storage          */
        chr($raw?0:1),
#    uint8_t u8Height;               /* Character height for storage         */
        chr($heightpx),
#    uint8_t u8FirstChar;            /* The first character available        */
        chr($first),
#    uint8_t u8LastChar;             /* The last character available         */
        chr($last);

    print B pack("S",scalar(@extras));
    print B map {pack "S",$_} @extras;
    print B map {pack "C",$_} @binoffsets;
    print B map {pack "C",$_} @bindata;
    close(B);
};

open (H,">",$file.".h")||die "Can't create $file.h: $!";
print H <<EOF;
#include "lcd/fonts.h"

extern const struct FONT_DEF Font_$fonts;
EOF
close(H);

print "\ndone.\n" if($verbose);
print "\n";
print "Original size: $origsize\n";
print "Simple compression: $c1size\n";
print( ($raw?"No":"PK")." compression: $c2size\n");
print "Maximum character size is: $heightb*$maxsz bytes\n";


exit(0);

# subs

sub pk_dedup {
	my $char=shift;
	my @echar=@{$char};

#   for (@echar){ print "dedup_in: $_\n"; };

	my $idx=0;
	while(++$idx<$#echar){
		# dupline code can't deal with all-0 or all-1 dupe lines
		next if ($echar[$idx]=~/^(.)(\1)+$/);

		if($echar[$idx-1] eq $echar[$idx]){
			my $dl=1;
			$dl++ while ($idx+$dl<$#echar && $echar[$idx] eq $echar[$idx+$dl]);

#			print "dupline found\n";
			if( $echar[$idx-1]=~ s/01/0[$dl]1/ ){
				$echar[$idx]="";
				@echar[$idx..($idx+$dl-1)]=();
			}elsif ($echar[$idx-1]=~ s/10/1[$dl]0/){
				$echar[$idx]="";
				@echar[$idx..($idx+$dl-1)]=();
			}else{
				die "Shouldn't happen: Can't encode dupline?";
			};
			$idx+=$dl; # Skip deduped lines.
		}
	}
	@echar=grep {defined $_} @echar;
	return \@echar;
};

sub pk_rle {
	my $char=shift;

	my $line=join("",@$char);;

	my @out;
	while($line=~/./){
		$line=~s/^(0*)(\[\d+\])?(1*)(\[\d+\])?//;
		push @out,length($1);
		push @out,$2 if defined $2;
		push @out,length($3);
		push @out,$4 if defined $4;
	};
	pop @out if ($out[$#out]==0); # Remove trailling zero

#	print "rle: ",join(",",@out),"\n";
	return @out;
};

###
### Encode a "long run", i.e. big number
###
sub pk_encode_long {
	my $n=shift;
	my @packed;

	while($n){
		unshift @packed,$n%16;
		$n=$n>>4;
	};

	for my $undef (1..$#packed){
		unshift @packed,0;
	};

	return @packed;
};

###
### Encode RLE data (with included repeat counts) into a nybble stream
###
### PK has "dyn" per-character, but for our font size encoding a dyn per
### character needs more space than it saves, so it's fixed for now.
###
sub pk_encode {
	my @out=@_;
	my $dyn=12;
	my @enc;

	for (@out){
		if($_ =~ /\[(\d+)\]/ ){
			if($1 == 1){
				push @enc,15;
			}else{
				my $n=$1-1; # this deviates from PK spec, i think.
				push @enc,14,pk_encode_long($1-1);
			};
		}elsif($_ == 0){ # Encoding a 0 will only happen at the start of
                         # character if "first pixel" is 1 instead of 0.
                         # HACK:  We transmit this fact to the decoder
                         #        by encoding a "14"-nibble which would be
                         #        illegal at this point anyway.
			push @enc,14;
		}elsif($_ <= $dyn){ # Short length
			push @enc,$_;
		}elsif($_ <= 16*(13-$dyn)+$dyn){ # Medium length
			my $b=($_-$dyn-1)&0x0f;
			my $a=(($_-$dyn-1)>>4)+$dyn+1;
			push @enc,$a,$b; # (16*($a-$dyn-1)+$b+$dyn+1
		}else{ # long length
			my $n=$_- (16*(13-$dyn)+$dyn) + 16;
			push @enc,pk_encode_long($n);
		};
	};
#	print "enc: ",join(",",@enc),"\n";
	return @enc;
};

sub make_bytes{
	my @enc=@_;
	my @out;

	while(@enc){
		push @enc,1 if($#enc==2);
		push @out,16*(shift@enc)+(shift@enc);
	};
	return @out;
};

sub do_pk {
	my $char=shift;
	my $size=scalar @$char * $heightb;
	print "Input char is $size bytes\n" if $verbose;

	$char=pk_dedup($char);
	
	if($verbose){
		for (@$char){
			print "dedup: $_\n";
		};
	};

	my @rle=pk_rle ($char);

	if($verbose){
		print "RLE: ",join(",",@rle),"\n";
	};

	my @enc=pk_encode (@rle);

	if($verbose){
		print "encoded stream: ",join(",",@enc),"\n";
	};

	return make_bytes(@enc);
};

sub getfontname {
	my $file = shift;
	use constant SEEK_SET => 0;
	use Encode qw(decode);
	my @font;

	open (my $fh,"<",$file) || die "Can't open $font: $!";

	my($buf,$str);

	sysread($fh,$buf,12); # OFFSET_TABLE
	my($maj,$min,$num,undef,undef,undef)=unpack("nnnnnn",$buf);

	die "It's not a truetype font!" if ($maj != 1 || $min != 0);

	for(1..$num){
		sysread($fh,$buf,16); # TABLE_DIRECTORY
		my($name,undef,$off1,$len)=unpack("A4NNN",$buf);
		if ($name eq "name"){
			seek($fh,$off1,SEEK_SET);
			sysread($fh,$buf,6);
			my(undef,$cnt,$off2)=unpack("nnn",$buf);
			sysread($fh,$buf,12*$cnt);
			while(length($buf)){
				my(undef,$enc,undef,$id,$len,$off3)=unpack("nnnnnn",$buf);
				substr($buf,0,12)="";
				seek($fh,$off1+$off2+$off3,SEEK_SET);
				sysread($fh,$str,$len);
				if($enc==1){
					$str=decode("UCS-2",$str);
				};
				# 0 	Copyright notice
				# 1 	Font Family name.
				# 2 	Font Subfamily name.
				# 3 	Unique font identifier.
				# 4 	Full font name.
				# 5 	Version string.
				# 6 	Postscript name for the font.
				# 7 	Trademark
				# 8 	Manufacturer Name.
				# 9 	Designer.
				# 10 	Description.
				# 11 	URL Vendor.
				# 12 	URL Designer.
				# 13 	License description
				# 14 	License information URL.
				$font[$id]=$str;
#				print "- $str\n";
			};
			last;
		};
	};
	my($fontname,$licence);
	$fontname=$font[1];

	if(defined $font[2]){
		$fontname.=" ".$font[2];
	}elsif (defined $font[4]){
		$fontname=$font[4];
	};
	$licence=$font[0]."\n";
	$licence.="\n".$font[13]."\n" if defined $font[13];
	$licence.="\nSee also: ".$font[14]."\n" if defined $font[14];
	if(wantarray()){
		return ($fontname,$licence);
	}else{
		return $fontname;
	};
};
######################################################################
our $bdf;
our %chars;
our $fallback;
sub init_bdf{
	($title,$licence)=($font,"<licence>");
	my($bb);

	open($bdf,"<",$font) || die "Can't open $font: $!";

	while(<$bdf>){
		chomp;
		/^PIXEL_SIZE (.*)/ && do { $heightpx=$1;$heightpx+=0;};
#		/^FONT_ASCENT (.*)/ && do {$fonta=$1};
#		/^FONT_DESCENT (\d+)/ && do {$fontd=$1;$byte=int(($fonta+$fontd-1)/8)+1;print "This will be a $byte byte font\n";};
#		/^DWIDTH (\d+) (\d+)/ && do {$width=$1;die "H-offset?" if $2!=0};
		/^FACE_NAME "(.*)"/ && do {$font=$1;};
		/^COPYRIGHT "(.*)"/ && do {$licence=$1;};
		/^FAMILY_NAME "(.*)"/ && do {$title=$1;};
		/^FONTBOUNDINGBOX (\d+) (\d+)/ && do {$bb="$1x$2";};
		/^DEFAULT_CHAR (\d+)/ && do {$fallback=$1;};
		
		last if /^ENDPROPERTIES/;
	};
	$title.="-".$bb if($bb);

	$fonts=$title;
	$fonts=~s/[ -]//g;

	my($bbw,$bbh,$bbx,$bby);
	my($ccode,$inchar,@bchar);
	while(<$bdf>){
		chomp;
		/^ENDCHAR/ && do {
            $bbh=$#bchar+1 if !$bbh;
			warn "Char $ccode has strange height?\n" if ($#bchar+1 != $bbh);
			for (1..$bby){
				push @bchar,("0"x$bbw);
			};
			for (@bchar){
				$_=("0"x$bbx).$_;
			};
			$inchar=0;


			my $tw=length($bchar[0]);
			my $th=$#bchar;

			my @tchar;
			@tchar=();
			for my $xw (1..$tw){
				my $pix="";
				for my $yw (0..$th){
					$pix.=substr($bchar[$yw],$xw-1,1);
				};
				push @tchar,$pix;
			};

			$chars{$ccode}=[@tchar];
#			print "Char: $ccode:\n",join("\n",@tchar),"\nEND\n";
			@bchar=();
		};
		if($inchar){
            my $x;
            if($inchar==2){
                $x=$_;
                $x=~y/ ./0/;
                $x=~y/xX\*/1/;
                $x=~y/01//cd;
                next if($x eq "");
                $bbw=length($x) if !$bbw;
            }else{
                $x=unpack("B*",pack("H*",$_));
                $x=substr($x,0,$bbw);
            };
			push @bchar,$x;
#			$x=~y/01/ */;
#			print $x,"\n";
			next;
		};

		/^BITMAP/ && do {$inchar=1;};
		/^XBITMAP/ && do {$inchar=2;($bbw,$bbh,$bbx,$bby)=(0)x4;};
		/^ENCODING (.*)/ && do {$ccode=$1; };
		/^BBX (\d+) (\d+) (\d+) ([-\d]+)/ && do {$bbw=$1;$bbh=$2;$bbx=$3;$bby=$4;};
	};

	close($bdf);

};

sub render_bdf{
	my $ccode=$charlist[shift];
#	print "Char: $ccode:\n";
	$ccode=$fallback if !defined $chars{$ccode};
	my $tchar=$chars{$ccode};
#	print join("\n",@{$tchar}),"\nEND\n";
	return @{$tchar};
};

######################################################################
our($charlist);
our($height,$width,$xoff);
our($mx,$my);
our($top,$bottom);
sub init_ttf {
	load GD;
	($height,$width,$xoff)=(100,5000,90);

	($title,$licence)=getfontname($font);
	die "Couldn't get font name?" if !defined $title;

	$title.=" ${size}pt";

	$fonts=$title;
	$fonts=~s/[ -]//g;
	$fonts=~s/Bitstream//;
	$fonts=~s/Sans//;
	$fonts=~s/Regular//;

	$charlist=join("",map {chr $_} @charlist);

### Get & optimize bounding box

	my $im = new GD::Image($width,$height);
	my $white = $im->colorAllocate(255,255,255);
	my $black = $im->colorAllocate(0,0,0);

	my @bounds = $im->stringFT(-$black, $font, $size, 0, 0, $xoff,$charlist);

	($mx,$my)=($bounds[2],$bounds[3]);
	($top,$bottom)=($bounds[7],$my);

	if(!defined $mx){
		die "GD::Image failed: $@\n";
	};

	die "Increase width" if $mx>$width;
	die "Increase height" if $my>$width;
	die "Increase xoff" if $bounds[7]<0;

	my $found;

# Cut whitespace at top
	do {
		$found=0;
		for my $x (0..$mx){
			if( $im->getPixel($x,$top) == 1){
				$found=1;last;
			}
		};
		$top++;
	}while($found==0);
	$top--;

# Cut whitespace at bottom.
	do {
		$found=0;
		for my $x (0..$mx){
			if( $im->getPixel($x,$bottom) == 1){
				$found=1;last;
			}
		};
		$bottom--;
	}while($found==0);
	$bottom++;

	$heightpx=$bottom-$top+1;

	print "Removed ",$top-$bounds[7],"px at top\n";
	print "Removed ",$my-$bottom,"px at bottom\n";

};

sub render_ttf{
	my $char=substr($charlist,shift,1);

	# create a new image
	my $im = new GD::Image(2*$height,$height);
	my $white = $im->colorAllocate(255,255,255);
	my $black = $im->colorAllocate(0,0,0);

	my @bounds = $im->stringFT(-$black, $font, $size, 0, 0, $xoff,$char.$charlist);

	my @char;
	for my $y ($top..$bottom){
		for my $x (0..($bounds[2]-$mx)){
			my $px= $im->getPixel($x,$y);
			$char[$x].=$px;
#			$px=~y/01/ */; print $px;
		};
#		print "<\n";
	};
	return @char;
};
