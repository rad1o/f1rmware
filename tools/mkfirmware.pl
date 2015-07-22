#!/usr/bin/env perl
#
# vim:set ts=4 sw=4:

use strict;

my $debug=0;
if($ARGV[0] eq "-d"){
    $debug=1;
   shift;
};

my @files=@ARGV;
my %menu;
my (@ticks,@inits);

$\="\n";

print "// Function definitions:";
my $menudef;
for my $file (@files){
	open(F,"<",$file) || die "Can't open $file: $!";
	while(<F>){
		chomp;
		s/\r$//;  # Dos line-end

		if(m!^\s*//#\s+(.*?)\s*$!){  # Menu definition
			$menudef=$1;
			next;
		};
		next if(/^\s*$/);
		next if(m!^\s*//!);
		if(m!^\s*void\s+([^(]+)\(!){ # A suitable function
			my $func=$1;
            if($func=~/^tick_/){
                push @ticks,$func;
                print "void $func(void);";
            };
            if($func=~/^init_/){
                push @inits,$func;
                print "void $func(void);";
            };
            if($debug){
                my $f="debug";
                $file =~ m!([^/.]+)\.! && do {$f=$1};
                $menudef="MENU $f $func";
            };
			if(defined $menudef){
				my @words=split(/\s+/,$menudef);
				if($words[0] ne "MENU"){
					warn "Not a menu definition?";
				};
				
				if($#words==1){
					$menu{$words[1]}=$func;
				}elsif($#words==2){
					$menu{$words[1]}{$words[2]}=$func;
				}else{
					warn "Couldn't handle $menudef";
				};
                print "void $func(void);";
			};
			$menudef=undef;
		};
	};
};

print "";
print "// Submenus:";

#use Data::Dumper; print Dumper \%menu;

for (sort keys %menu){
    if(ref $menu{$_} eq "HASH"){
        printf "static const struct MENU submenu_${_}={";
        print qq! "$_", {!;
        for my $entry(sort keys %{$menu{$_}}){
            print qq!\t{ "$entry", &$menu{$_}{$entry}},!;
        };
        print qq!\t{NULL,NULL}!;
        print "}};";
    };
};

print "";

for (sort keys %menu){
    if(ref $menu{$_} eq "HASH"){
        print qq!void run_submenu_$_(void) {!;
        print qq!\thandleMenu(&submenu_$_);!;
        print qq!}!;
    };
};

print "";
print "// Main menu:";
printf "static const struct MENU menu_main={";
print qq! "Menu:", {!;
for (sort keys %menu){
    if(ref $menu{$_} eq "HASH"){
        print qq!\t{ "$_", &run_submenu_$_},!;
    }else{
        print qq!\t{ "$_", &$menu{$_}},!;
    };
};
print qq!\t\t{NULL,NULL}!;
print "}};";

print "";
print "// Tick & init functions:";
print qq!inline void generated_tick(void) {!;
for (sort @ticks){
    print qq!\t$_();!;
};
print qq!}!;

print qq!inline void generated_init(void) {!;
for (sort @inits){
    print qq!\t$_();!;
};
print qq!}!;

